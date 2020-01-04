/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2020, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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
#include "audio.h"

#if PAL_HAS_NATIVEMIDI
#include "native_midi/native_midi.h"
#endif

typedef struct tagMIDIPLAYER
{
    AUDIOPLAYER_COMMONS;
    
    BOOL                        fReady;
#if PAL_HAS_NATIVEMIDI
    NativeMidiSong             *pMidi;
#endif
    int                         iVolume;
} MIDIPLAYER, *LPMIDIPLAYER;

static void
MIDI_SetVolume(
    void      *object,
	int       iVolume
)
{
#if PAL_HAS_NATIVEMIDI
    LPMIDIPLAYER player = (LPMIDIPLAYER)object;
    if(!player)
        return;
	player->iVolume = iVolume;
	native_midi_setvolume(player->pMidi, iVolume * 127 / PAL_MAX_VOLUME);
#endif
}

static VOID
MIDI_FillBuffer(
    VOID       *object,
    LPBYTE      stream,
    INT         len
    )
{
    return;
}

static BOOL
MIDI_Stop(
    void      *object,
    FLOAT fFadeTime
)
{
#if PAL_HAS_NATIVEMIDI
    LPMIDIPLAYER player = (LPMIDIPLAYER)object;
    if(!player)
        return FALSE;

    native_midi_stop(player->pMidi);
    native_midi_freesong(player->pMidi);
    player->pMidi = NULL;
    player->iVolume = -1;
#endif
    return TRUE;
}

static VOID MIDI_Shutdown(
    VOID *object
)
{
#if PAL_HAS_NATIVEMIDI
    MIDI_Stop(object, 0);
#endif
}

static BOOL
MIDI_Play(
    void      *object,
	int       iNumRIX,
	BOOL      fLoop,
    FLOAT fFadeTime
)
{
#if PAL_HAS_NATIVEMIDI
    assert(iNumRIX > 0);

    LPMIDIPLAYER player = (LPMIDIPLAYER)object;
    if(!player)
        return FALSE;

	if (native_midi_active(player->pMidi) && iNumRIX == player->iVolume)
	{
		return TRUE;
	}

    MIDI_Stop(object, fFadeTime);

	if (gConfig.fIsWIN95)
	{
		player->pMidi = native_midi_loadsong(UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath, PAL_va(1, "Musics%s%.3d.mid", PAL_NATIVE_PATH_SEPARATOR, iNumRIX)));
    }
    else
    {
		FILE    *fp  = NULL;
		uint8_t *buf = NULL;
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
			SDL_RWops *rw = SDL_RWFromConstMem(buf, size);
			player->pMidi = native_midi_loadsong_RW(rw);
			SDL_RWclose(rw);
			free(buf);
		}
	}

	if (player->pMidi)
	{
		MIDI_SetVolume(player, player->iVolume);
		native_midi_start(player->pMidi, fLoop);
		player->iVolume = iNumRIX;
	}
#endif
    return TRUE;
}

LPAUDIOPLAYER
MIDI_Init(
    VOID
)
{
#if PAL_HAS_NATIVEMIDI
    LPMIDIPLAYER player;
    if ((player = (LPMIDIPLAYER)malloc(sizeof(MIDIPLAYER))) != NULL)
    {
        memset(player, 0, sizeof(MIDIPLAYER));

        player->FillBuffer = MIDI_FillBuffer;
        player->Play = MIDI_Play;
        player->Stop = MIDI_Stop;
        player->Shutdown = MIDI_Shutdown;
        player->SetVolume = MIDI_SetVolume;
        
        player->pMidi = NULL;
        player->iVolume = PAL_MAX_VOLUME;
    }
    return (LPAUDIOPLAYER)player;
#else
    return NULL;
#endif
}
