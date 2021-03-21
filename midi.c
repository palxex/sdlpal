/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2021, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "main.h"

#define TSF_IMPLEMENTATION
#include "tsf.h"
#define TML_IMPLEMENTATION
#include "tml.h"

static int  g_iMidiCurrent = -1;
static NativeMidiSong *g_pMidi = NULL;
static int  g_iMidiVolume = PAL_MAX_VOLUME;

static tsf* g_TinySoundFont;
static double g_Msec;
static tml_message *g_MidiMessage, *g_curMidiMsg;
static BOOL g_bNoteOn;

void
MIDI_SetVolume(
	int       iVolume
)
{
	if (gConfig.eMIDISynth == SYNTH_TINYSOUNDFONT) {

	}else{
#if PAL_HAS_NATIVEMIDI
	g_iMidiVolume = iVolume;
	if (g_pMidi)
	{
		native_midi_setvolume(g_pMidi, iVolume * 127 / PAL_MAX_VOLUME);
	}
#endif
	}
}

void TSF_LoadMIDI(int iNumRIX)
{
	tml_free(g_curMidiMsg);
	g_MidiMessage = NULL;
	g_Msec = 0;

	if (gConfig.fIsWIN95)
		g_curMidiMsg = g_MidiMessage = tml_load_filename(UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath, PAL_va(1, "Musics%s%.3d.mid", PAL_NATIVE_PATH_SEPARATOR, iNumRIX)));

	if (!g_MidiMessage)
	{
		FILE* fp = NULL;
		uint8_t* buf = NULL;
		int      size;

		if ((fp = UTIL_OpenFile("midi.mkf")) != NULL)
		{
			if ((size = PAL_MKFGetChunkSize(iNumRIX, fp)) > 0 &&
				(buf = (uint8_t*)UTIL_malloc(size)))
			{
				PAL_MKFReadChunk(buf, size, iNumRIX, fp);
			}
			fclose(fp);
		}

		if (buf)
		{
			g_curMidiMsg = g_MidiMessage = tml_load_memory(buf, size);
			free(buf);
		}
	}
}

PAL_C_LINKAGE
void
MIDI_FillBuffer(
	LPBYTE      stream,
	INT         len
)
{
	if (gConfig.eMIDISynth == SYNTH_TINYSOUNDFONT && g_bNoteOn) {
 		SDL_AudioSpec* pSpec = AUDIO_GetDeviceSpec();
		int SampleBlock, SampleCount = (len / (pSpec->channels * sizeof(short)));
		for (SampleBlock = TSF_RENDER_EFFECTSAMPLEBLOCK; SampleCount; SampleCount -= SampleBlock, stream += (SampleBlock * (pSpec->channels * sizeof(short))))
		{
			if (SampleBlock > SampleCount) SampleBlock = SampleCount;

			for (g_Msec += SampleBlock * (1000.0 / pSpec->freq); g_MidiMessage && g_Msec >= g_MidiMessage->time; g_MidiMessage = g_MidiMessage->next)
			{
				switch (g_MidiMessage->type)
				{
				case TML_PROGRAM_CHANGE: 
					tsf_channel_set_presetnumber(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->program, (g_MidiMessage->channel == 9));
					break;
				case TML_NOTE_ON: 
					tsf_channel_note_on(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->key, g_MidiMessage->velocity / 127.0f);
					break;
				case TML_NOTE_OFF: 
					tsf_channel_note_off(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->key);
					break;
				case TML_PITCH_BEND: 
					tsf_channel_set_pitchwheel(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->pitch_bend);
					break;
				case TML_CONTROL_CHANGE:
					tsf_channel_midi_control(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->control, g_MidiMessage->control_value);
					break;
				}
			}

			tsf_render_short(g_TinySoundFont, (short*)stream, SampleBlock, 0);
		}
		if (g_MidiMessage == NULL)
			TSF_LoadMIDI(g_iMidiCurrent);
	}
}

void
MIDI_Play(
	int       iNumRIX,
	BOOL      fLoop
)
{
	if (gConfig.eMIDISynth == SYNTH_TINYSOUNDFONT) {
		if (g_bNoteOn && iNumRIX == g_iMidiCurrent)
		{
			return;
		}
		if (iNumRIX <= 0)
		{
			return;
		}

		if (!g_TinySoundFont)
		{
			g_TinySoundFont = tsf_load_filename(UTIL_IsAbsolutePath(gConfig.pszSoundFont) ? gConfig.pszSoundFont : UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath, gConfig.pszSoundFont));
			tsf_channel_set_bank_preset(g_TinySoundFont, 9, 128, 0);
			SDL_AudioSpec* pSpec = AUDIO_GetDeviceSpec();
			tsf_set_output(g_TinySoundFont, TSF_STEREO_INTERLEAVED, pSpec->freq, 0.0f);
		}
		if (!g_TinySoundFont)
		{
			TerminateOnError("Could not load SoundFont\n");
			return;
		}
		g_bNoteOn = FALSE;
		g_iMidiCurrent = -1;
		tsf_reset(g_TinySoundFont);

		TSF_LoadMIDI(iNumRIX);

		if (g_MidiMessage)
		{
			MIDI_SetVolume(g_iMidiVolume);
			g_iMidiCurrent = iNumRIX;
			g_bNoteOn = TRUE;
		}
	}else{
#if PAL_HAS_NATIVEMIDI
		if (!native_midi_detect())
			return;

	if (native_midi_active(g_pMidi) && iNumRIX == g_iMidiCurrent)
	{
		return;
	}

	native_midi_stop(g_pMidi);
	native_midi_freesong(g_pMidi);
	g_pMidi = NULL;
	g_iMidiCurrent = -1;

	if (!AUDIO_MusicEnabled() || iNumRIX <= 0)
	{
		return;
	}

	if (gConfig.fIsWIN95)
	{
		g_pMidi = native_midi_loadsong(UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath, PAL_va(1, "Musics%s%.3d.mid", PAL_NATIVE_PATH_SEPARATOR, iNumRIX)));
	}

	if (!g_pMidi)
	{
		FILE* fp = NULL;
		uint8_t* buf = NULL;
		int      size;

		if ((fp = UTIL_OpenFile("midi.mkf")) != NULL)
		{
			if ((size = PAL_MKFGetChunkSize(iNumRIX, fp)) > 0 &&
				(buf = (uint8_t*)UTIL_malloc(size)))
			{
				PAL_MKFReadChunk(buf, size, iNumRIX, fp);
			}
			fclose(fp);
		}

		if (buf)
		{
			SDL_RWops* rw = SDL_RWFromConstMem(buf, size);
			g_pMidi = native_midi_loadsong_RW(rw);
			SDL_RWclose(rw);
			free(buf);
		}
	}

	if (g_pMidi)
	{
		MIDI_SetVolume(g_iMidiVolume);
		native_midi_start(g_pMidi, fLoop);
		g_iMidiCurrent = iNumRIX;
	}
#endif
	}
}

void
MIDI_Shutdown(
)
{
	if (gConfig.eMIDISynth == SYNTH_TINYSOUNDFONT) {
        g_bNoteOn = FALSE;
		tsf_close(g_TinySoundFont);
        g_TinySoundFont = NULL;
		tml_free(g_curMidiMsg);
        g_curMidiMsg = g_MidiMessage = NULL;
	}
}
