/* 
 * Just an empty placeholder for the original pcecd.cpp for us in EMSCRIPTEN.
 *
 * Mednafen - Multi-system Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2004 Ki
 *  Copyright (C) 2007-2011 Mednafen Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <mednafen/mednafen.h>
#include <mednafen/cdrom/cdromif.h>
#include <mednafen/cdrom/SimpleFIFO.h>
#include "pcecd.h"

namespace MDFN_IEN_PCE
{

static void (*IRQCB)(bool asserted);

// Settings:
static double CDDABaseVolume;
static double ADPCMBaseVolume;
static bool ADPCMExtraPrecision;


static bool	bBRAMEnabled;
static uint8	_Port[15];
static uint8 	ACKStatus;

static SimpleFIFO<uint8> SubChannelFIFO(16);

static int32* ADPCMBuf;
static int16 RawPCMVolumeCache[2];

static int32 ClearACKDelay;

static int32 lastts;
static int32 scsicd_ne;

// ADPCM variables and whatnot
#define ADPCM_DEBUG(x, ...) {  /*printf("[Half=%d, End=%d, Playing=%d] "x, ADPCM.HalfReached, ADPCM.EndReached, ADPCM.Playing, ## __VA_ARGS__);*/  }


typedef struct
{
 uint8    *RAM;	// = NULL; //0x10000;
 uint16   Addr;
 uint16   ReadAddr;
 uint16   WriteAddr;
 uint16   LengthCount;

 bool HalfReached;
 bool EndReached;
 bool Playing;

 uint8 LastCmd;
 uint32 SampleFreq;

 uint8 PlayBuffer;
 uint8 ReadBuffer;
 int32 ReadPending;
 int32 WritePending;
 uint8 WritePendingValue;

 uint32 PlayNibble;


 int64 bigdivacc;
 int64 bigdiv;
 //
 //
 //
 int32 last_pcm;
 int32 integrate_accum;
 int64 lp1p_fstate;
 int64 lp2p_fstate[3];
} ADPCM_t;

static ADPCM_t ADPCM;

uint32 PCECD_GetRegister(const unsigned int id, char *special, const uint32 special_len)
{
 return 0;
}

void PCECD_SetRegister(const unsigned int id, const uint32 value)
{
}

void ADPCM_PeekRAM(uint32 Address, uint32 Length, uint8 *Buffer)
{
 while(Length--)
 {
  Address &= 0xFFFF;
  *Buffer = ADPCM.RAM[Address];
  Address++;
  Buffer++;
 }
}

void ADPCM_PokeRAM(uint32 Address, uint32 Length, const uint8 *Buffer)
{
 while(Length--)
 {
  Address &= 0xFFFF;
  ADPCM.RAM[Address] = *Buffer;
  Address++;
  Buffer++;
 }
}

bool PCECD_SetSettings(const PCECD_Settings *settings)
{
	return true;
}

void PCECD_Init(const PCECD_Settings *settings, void (*irqcb)(bool), double master_clock, int32* adbuf, int32* hrbuf_l, int32* hrbuf_r) {}

void PCECD_Close(void) {}

int32 PCECD_Power(uint32 timestamp)
{
	return 0x7FFFFFFF;
}

bool PCECD_IsBRAMEnabled()
{
	return bBRAMEnabled;
}

MDFN_FASTCALL uint8 PCECD_Read(uint32 timestamp, uint32 A, int32 &next_event, const bool PeekMode)
{
 next_event = (0x7FFFFFFF);
 return 0;
}

MDFN_FASTCALL int32 PCECD_Write(uint32 timestamp, uint32 physAddr, uint8 data)
{
	return 0x7FFFFFFF;
}

void PCECD_ProcessADPCMBuffer(const uint32 rsc) {}

MDFN_FASTCALL int32 PCECD_Run(uint32 in_timestamp)
{
 return 0x7FFFFFFF;
}

void PCECD_ResetTS(uint32 ts_base) {}

void PCECD_StateAction(StateMem *sm, const unsigned load, const bool data_only) { }

}
