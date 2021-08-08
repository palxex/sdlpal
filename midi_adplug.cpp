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

#include "adplug/opl.h"
#include "adplug/emuopls.h"
#include "adplug/surroundopl.h"
#include "adplug/convertopl.h"
#include "adplug/mid.h"

typedef struct tagADPLUGPLAYER
{
	AUDIOPLAYER_COMMONS;

	Copl                      *opl;
    CmidPlayer                *mid;

   BYTE                       buf[(PAL_MAX_SAMPLERATE + 69) / 70 * sizeof(short) * 2];
   LPBYTE                     pos;
} ADPLUGPLAYER, *LPADPLUGPLAYER;

static VOID ADPLUG_Close(
	LPADPLUGPLAYER player
)
{
	if (player->opl)
	{
		delete player->opl;
		player->opl = NULL;
	}
	if (player->mid)
	{
		delete player->mid;
		player->mid = NULL;
	}
}

int getsampsize() {
	return 2 * 2;
}

static VOID
ADPLUG_FillBuffer(
	VOID       *object,
	LPBYTE      stream,
	INT         len
)
{
	LPADPLUGPLAYER player = (LPADPLUGPLAYER)object;
   if (!player || player->iMusic <= 0)
   {
      //
      // No current playing music
      //
      return;
   }

   static long	minicnt = 0;
   long		i, towrite = len / getsampsize();
   char* pos = (char*)stream;

	while (towrite > 0) {
	   while (minicnt < 0) {
		   minicnt += gConfig.iSampleRate;
		   if (!player->mid->update()) {
				if(!player->fLoop) {
					player->iMusic = -1;
					return;
				}
				player->mid->rewind(0);
				if (!player->mid->update()) {
				   player->iMusic = -1;
				   return;
				}
		   }
	   }
	   i = min(towrite, (long)(minicnt / player->mid->getrefresh() + 4) & ~3);
	   player->opl->update((short*)pos, i);
	   pos += i * getsampsize(); towrite -= i;
	   minicnt -= (long)(player->mid->getrefresh() * i);
   }
}

static VOID
ADPLUG_Shutdown(
	VOID       *object
	)
{
	LPADPLUGPLAYER player = (LPADPLUGPLAYER)object;
	if (player)
	{
		ADPLUG_Close(player);
		free(player);
	}
}

static BOOL
ADPLUG_Play(
	VOID       *object,
	INT         iNum,
	BOOL        fLoop,
	FLOAT       flFadeTime
	)
{
	LPADPLUGPLAYER player = (LPADPLUGPLAYER)object;

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

	player->iMusic = iNum;

	if (iNum > 0)
	{
		int loaded = 0;
		if (gConfig.fIsWIN95)
			loaded = player->mid->load(PAL_CombinePath(0, gConfig.pszGamePath, PAL_va(1, "Musics%s%.3d.mid", PAL_NATIVE_PATH_SEPARATOR, iNum)), CProvider_Filesystem());
		else
		{
			FILE    *fp  = NULL;
			uint8_t *buf = NULL;
			int      size;

			if ((fp = UTIL_OpenFile("midi.mkf")) != NULL)
			{
				if ((size = PAL_MKFGetChunkSize(iNum, fp)) > 0 &&
					(buf = (uint8_t*)UTIL_malloc(size)))
				{
					PAL_MKFReadChunk(buf, size, iNum, fp);
				}
				fclose(fp);
			}

			if (buf)
			{
				loaded = player->mid->loadfrommem(buf, size);
				free(buf);
			}
		}

		return loaded;
	}
	else
	{
		return TRUE;
	}
}

LPAUDIOPLAYER
ADPLUGMIDI_Init(
	VOID
)
{
	LPADPLUGPLAYER player;
	if ((player = (LPADPLUGPLAYER)malloc(sizeof(ADPLUGPLAYER))) != NULL)
	{
		memset(player, 0, sizeof(ADPLUGPLAYER));

		player->FillBuffer = ADPLUG_FillBuffer;
		player->Play = ADPLUG_Play;
		player->Shutdown = ADPLUG_Shutdown;

	auto chip = (Copl::ChipType)gConfig.eOPLChip;
	if (chip == Copl::TYPE_OPL2 && gConfig.fUseSurroundOPL)
	{
		chip = Copl::TYPE_DUAL_OPL2;
	}

	Copl* opl = CEmuopl::CreateEmuopl((OPLCORE::TYPE)gConfig.eOPLCore, chip, gConfig.iOPLSampleRate);
	if (NULL == opl)
	{
		delete player;
		return NULL;
	}

	if (gConfig.fUseSurroundOPL)
	{
		Copl* tmpopl = new CSurroundopl(gConfig.iOPLSampleRate, gConfig.iSurroundOPLOffset, opl);
		if (NULL == tmpopl)
		{
			delete opl;
			delete player;
			return NULL;
		}
		opl = tmpopl;
	}

	player->opl = new CConvertopl(opl, true, gConfig.iAudioChannels == 2);
	if (player->opl == NULL)
	{
		delete opl;
		delete player;
		return NULL;
	}

	player->mid = new CmidPlayer(player->opl);
	if (player->mid == NULL)
	{
		delete player->opl;
		delete player;
		return NULL;
	}

		player->iMusic = -1;
		player->fLoop = FALSE;
	}
	return (LPAUDIOPLAYER)player;
}
