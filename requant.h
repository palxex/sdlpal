#ifndef REQUANT_H
#define REQUANT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Quality levels – names directly describe the algorithm */
#define REQ_QUALITY_MIN              0
#define REQ_QUALITY_TRUNCATE         0   /* Direct truncation, no dither, no feedback */
#define REQ_QUALITY_ERROR_ONLY       1   /* First‑order error feedback only, no dither */
#define REQ_QUALITY_ERROR_TPDF       2   /* First‑order error feedback + TPDF dither */
#define REQ_QUALITY_MU_LAW           3   /* μ‑law non‑uniform quantization, no dither, no feedback */
#define REQ_QUALITY_NOISE_SHAPE_3RD  4   /* Third‑order noise shaping, no dither */
#define REQ_QUALITY_MAX              4

/**
 * Universal 16‑bit signed PCM requantization function.
 *
 * @param src_buf         Input 16‑bit signed PCM data.
 * @param src_channels    Number of input channels.
 * @param src_samples     Number of frames per channel.
 * @param dst_buf         Output byte buffer.
 * @param dst_channels    Number of output channels.
 * @param dst_bytes       Total size of output buffer in bytes (used to prevent out-of-bounds writes).
 * @param dst_bit         Target bit depth (1 ~ 32).
 * @param dst_signed      Whether output integer is signed (false => unsigned bias; ignored if dst_float is true).
 * @param dst_float       Whether output is 32-bit float (true => normalized to [-1.0, 1.0]).
 * @param is_littleendian Output byte order (true = little-endian, false = big-endian; no effect for 8-bit or float).
 * @param quality         Quality level: REQ_FAST / REQ_MEDIUM / REQ_HIGH (ignored when upscaling).
 */
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
);

#endif /* REQUANT_H */