/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2021, SDLPAL development team.
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

#include "util.h"
#include "global.h"
#include "palcfg.h"
#include "audio.h"
#include "players.h"

#if PAL_HAS_FLUIDMIDI 1

#include "fluidlite.h"

typedef struct tagMIDPLAYER
{
	AUDIOPLAYER_COMMONS;

	fluid_settings_t	*pSettings;
	fluid_synth_t       *pSynth;
} FLUIDPLAYER, *LPFLUIDPLAYER;

static VOID FLUID_Close(
	LPFLUIDPLAYER player
	)
{
	//never change destroy sequence OR IT WILL CRASH
	if (player->pSynth)
	{
		delete_fluid_synth(player->pSynth);
		player->pSynth = NULL;
	}
	if (player->pSettings)
	{
		delete_fluid_settings(player->pSettings);
		player->pSettings = NULL;
	}
}

static VOID
FLUID_FillBuffer(
	VOID       *object,
	LPBYTE      stream,
	INT         len
	)
{
	LPFLUIDPLAYER player = (LPFLUIDPLAYER)object;
	if (player->pSynth) {
		if (!mad_isPlaying(player->pMP3) && player->fLoop)
		{
			mad_seek(player->pMP3, 0);
			mad_start(player->pMP3);
		}

		if (mad_isPlaying(player->pMP3))
			mad_getSamples(player->pMP3, stream, len);
	}
}

static VOID
FLUID_Shutdown(
	VOID       *object
	)
{
	if (object)
	{
		MP3_Close((LPMP3PLAYER)object);
		free(object);
	}
}

static BOOL
FLUID_Play(
	VOID       *object,
	INT         iNum,
	BOOL        fLoop,
	FLOAT       flFadeTime
	)
{
	LPMP3PLAYER player = (LPMP3PLAYER)object;

	//
	// Check for NULL pointer.
	//
	if (player == NULL)
	{
		return FALSE;
	}

	player->fLoop = fLoop;

	if (iNum == player->iMusic)
	{
		return TRUE;
	}

	MP3_Close(player);

	player->iMusic = iNum;

	if (iNum > 0)
	{
		player->pMP3 = mad_openFile(UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath, PAL_va(1, "mp3%s%.2d.mp3", PAL_NATIVE_PATH_SEPARATOR, iNum)), AUDIO_GetDeviceSpec(), gConfig.iResampleQuality);

		if (player->pMP3)
		{
			mad_start(player->pMP3);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return TRUE;
	}
}

LPAUDIOPLAYER
FLUID_Init(
	VOID
)
{
	LPMP3PLAYER player;
	if ((player = (LPMP3PLAYER)malloc(sizeof(MP3PLAYER))) != NULL)
	{
		player->FillBuffer = MP3_FillBuffer;
		player->Play = MP3_Play;
		player->Shutdown = MP3_Shutdown;

		player->pMP3 = NULL;
		player->iMusic = -1;
		player->fLoop = FALSE;
	}
	return (LPAUDIOPLAYER)player;
}

#else

LPAUDIOPLAYER
FLUID_Init(
	VOID
)
{
	return NULL;
}

#endif
