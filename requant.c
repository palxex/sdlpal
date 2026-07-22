#include "requant.h"
#include <string.h>
#include <math.h>

#define MAX_AUDIO_CHANNELS 8

typedef struct {
    int32_t error[3];   /* error[0] current, error[1] previous, error[2] second previous */
    uint32_t seed;
} QuantState;

void s16_quant_to_bit(
    const int16_t* src_buf,
    int src_channels,
    size_t src_samples,
    uint8_t* dst_buf,
    int dst_channels,
    size_t dst_bytes,
    int dst_bit,
    bool dst_signed,
    bool dst_float,
    bool is_littleendian,
    int quality
) {
    if (!src_buf || !dst_buf || dst_bit < 1 || dst_bit > 32) return;
    if (src_channels < 1 || src_channels > MAX_AUDIO_CHANNELS) return;
    if (dst_channels < 1 || dst_channels > MAX_AUDIO_CHANNELS) return;
    if (src_samples == 0 || dst_bytes == 0) return;

    int bytes_per_sample = (dst_bit + 7) >> 3;
    if (bytes_per_sample < 1) bytes_per_sample = 1;

    size_t max_frames = dst_bytes / (dst_channels * bytes_per_sample);
    if (max_frames == 0) return;
    if (src_samples > max_frames) src_samples = max_frames;

    QuantState states[MAX_AUDIO_CHANNELS];
    memset(states, 0, sizeof(states));

    uint32_t half_range_u;
    if (dst_bit == 32) half_range_u = 0x80000000u;
    else half_range_u = 1u << (dst_bit - 1);

    const int32_t max_val = (int32_t)(half_range_u - 1u);
    const int32_t min_val = -(int32_t)half_range_u;

    float float_divisor;
    if (dst_bit == 32) float_divisor = 2147483648.0f;
    else float_divisor = (float)(1u << (dst_bit - 1));

    for (size_t f = 0; f < src_samples; f++) {
        size_t in_base = f * src_channels;
        int32_t in_samples[MAX_AUDIO_CHANNELS];
        for (int c = 0; c < src_channels; c++) {
            in_samples[c] = (int32_t)src_buf[in_base + c];
        }

        for (int oc = 0; oc < dst_channels; oc++) {
            int32_t sample;
            if (src_channels == dst_channels) {
                sample = in_samples[oc];
            }
            else if (src_channels == 1 && dst_channels == 2) {
                sample = in_samples[0];
            }
            else if (src_channels == 2 && dst_channels == 1) {
                sample = (in_samples[0] + in_samples[1]) >> 1;
            }
            else {
                sample = in_samples[oc % src_channels];
            }

            QuantState* st = &states[oc];
            int32_t out;

            if (dst_bit >= 16) {
                int shift = dst_bit - 16;
                out = sample << shift;
            }
            else {
                int effective_quality = quality;
                if (quality == REQ_QUALITY_MU_LAW && dst_bit < 8) {
                    effective_quality = REQ_QUALITY_ERROR_ONLY;
                }

                int shift = 16 - dst_bit;

                if (effective_quality == REQ_QUALITY_MU_LAW) {
                    float norm = sample / 32768.0f;
                    float compressed;
                    if (norm >= 0)
                        compressed = logf(1.0f + 255.0f * norm) / logf(1.0f + 255.0f);
                    else
                        compressed = -logf(1.0f - 255.0f * norm) / logf(1.0f + 255.0f);
                    int32_t quantized;
                    if (dst_signed)
                        quantized = (int32_t)(compressed * half_range_u);
                    else
                        quantized = (int32_t)(compressed * half_range_u + half_range_u);
                    out = quantized;
                    if (out > max_val) out = max_val;
                    if (out < min_val) out = min_val;
                }
                else {
                    if (effective_quality == REQ_QUALITY_ERROR_TPDF) {
                        st->seed = st->seed * 1664525u + 1013904223u;
                        int32_t r1 = (int32_t)((st->seed >> 16) & 0x7fff);
                        st->seed = st->seed * 1664525u + 1013904223u;
                        int32_t r2 = (int32_t)((st->seed >> 16) & 0x7fff);
                        int32_t tpdf = (r1 - r2) >> (16 - shift);
                        sample += tpdf;
                    }

                    if (effective_quality == REQ_QUALITY_ERROR_ONLY ||
                        effective_quality == REQ_QUALITY_ERROR_TPDF) {
                        sample += st->error[0];
                    }
                    else if (effective_quality == REQ_QUALITY_NOISE_SHAPE_3RD) {
                        /* Optimized 3rd-order coefficients: 2.5, -2, 0.5 (scaled to integer) */
                        sample += (int32_t)((int64_t)25 * st->error[0] - (int64_t)20 * st->error[1] + (int64_t)5 * st->error[2]) / 10;
                    }

                    if (sample > 32767) sample = 32767;
                    if (sample < -32768) sample = -32768;

                    out = sample >> shift;
                    if (out > max_val) out = max_val;
                    if (out < min_val) out = min_val;

                    if (effective_quality == REQ_QUALITY_ERROR_ONLY ||
                        effective_quality == REQ_QUALITY_ERROR_TPDF ||
                        effective_quality == REQ_QUALITY_NOISE_SHAPE_3RD) {
                        int32_t reconstructed = out << shift;
                        int32_t err = sample - reconstructed;
                        if (effective_quality == REQ_QUALITY_ERROR_ONLY ||
                            effective_quality == REQ_QUALITY_ERROR_TPDF) {
                            int leak = (dst_bit <= 6) ? 85 : 96;
                            st->error[0] = (err * leak) >> 7;
                        }
                        else if (effective_quality == REQ_QUALITY_NOISE_SHAPE_3RD) {
                            /* Shift history */
                            st->error[2] = st->error[1];
                            st->error[1] = st->error[0];
                            st->error[0] = err;
                            /* Apply leak 0.95 and clamp to prevent runaway */
                            st->error[0] = (int32_t)((int64_t)st->error[0] * 95 / 100);
                            /* Limit error to ±32768 to avoid overflow in next samples */
                            if (st->error[0] > 32767) st->error[0] = 32767;
                            if (st->error[0] < -32768) st->error[0] = -32768;
                        }
                    }
                }
            }

            size_t out_pos = (f * dst_channels + oc) * bytes_per_sample;
            if (out_pos + bytes_per_sample > dst_bytes) continue;

            if (dst_float) {
                float fval = (float)out / float_divisor;
                memcpy(dst_buf + out_pos, &fval, 4);
            }
            else {
                uint64_t uval;
                if (dst_signed) {
                    uval = (uint64_t)(int64_t)out;
                }
                else {
                    uval = (uint64_t)((int64_t)out + (int64_t)half_range_u);
                }
                int align_shift = (bytes_per_sample * 8) - dst_bit;
                uval <<= align_shift;
                if (is_littleendian) {
                    for (int b = 0; b < bytes_per_sample; b++) {
                        dst_buf[out_pos + b] = (uint8_t)((uval >> (b * 8)) & 0xFFu);
                    }
                }
                else {
                    for (int b = 0; b < bytes_per_sample; b++) {
                        dst_buf[out_pos + b] = (uint8_t)((uval >> ((bytes_per_sample - 1 - b) * 8)) & 0xFFu);
                    }
                }
            }
        }
    }
}