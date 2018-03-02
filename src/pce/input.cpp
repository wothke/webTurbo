/* Mednafen - Multi-system Emulator
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

#include "endian.h"	// EMSCRIPTEN

#include "pce.h"
#include "input.h"
#include "huc.h"

#include "vce.h"	// For debugging only(but not the debugger :b)

#include "input/gamepad.h"
#include "input/mouse.h"
#include "input/tsushinkb.h"

#include <trio/trio.h>

// XXXX FIXME should not be needed for HES at all

namespace MDFN_IEN_PCE
{

enum
{
 PCEINPUT_NONE = 0,
 PCEINPUT_GAMEPAD = 1,
 PCEINPUT_MOUSE = 2,
 PCEINPUT_TSUSHINKB = 3,
};

//PCE_Input_Device::PCE_Input_Device(int which)
//{
//
//}

PCE_Input_Device::~PCE_Input_Device()
{

}

void PCE_Input_Device::Power(int32 timestamp)
{


}

void PCE_Input_Device::TransformInput(uint8* data, const bool DisableSR)
{


}

void PCE_Input_Device::AdjustTS(int32 delta)
{

}

void PCE_Input_Device::Write(int32 timestamp, bool old_SEL, bool new_SEL, bool old_CLR, bool new_CLR)
{

}

uint8 PCE_Input_Device::Read(int32 timestamp)
{
 return(0xF);
}

void PCE_Input_Device::Update(const uint8* data)
{

}

int PCE_Input_Device::StateAction(StateMem *sm, int load, int data_only, const char *section_name)
{
 return(1);
}

static PCE_Input_Device *devices[5] = { NULL };
static bool MultiTapEnabled = true;
static int InputTypes[5] = { 0 };
static uint8 *data_ptr[5];
static bool SEL, CLR;
static uint8 read_index = 0;
static bool DisableSR = false;

static void RemakeDevices(int which = -1)
{
 int s = 0;
 int e = 5;

 if(which != -1)
 {
  s = which;
  e = which + 1;
 }

 for(int i = s; i < e; i++)
 {
  if(devices[i])
   delete devices[i];
  devices[i] = NULL;

  switch(InputTypes[i])
  {
   default:
   case PCEINPUT_NONE: break;
   case PCEINPUT_GAMEPAD: devices[i] = PCEINPUT_MakeGamepad(); break;
   case PCEINPUT_MOUSE: devices[i] = PCEINPUT_MakeMouse(); break;
   case PCEINPUT_TSUSHINKB: devices[i] = PCEINPUT_MakeTsushinKB(); break;
  }
 }
}


static void SyncSettings(void);

void PCEINPUT_SettingChanged(const char *name)
{
 SyncSettings();
}

void PCEINPUT_Init(void)
{
 SyncSettings();
 RemakeDevices();
}

void PCEINPUT_TransformInput(void)
{
 for(unsigned i = 0; i < 5; i++)
 {
  if(devices[i])
   devices[i]->TransformInput(data_ptr[i], DisableSR);
 }
}

void PCEINPUT_SetInput(unsigned port, const char *type, uint8 *ptr)
{
 assert(port < 5);

 if(!strcmp(type, "gamepad"))
  InputTypes[port] = PCEINPUT_GAMEPAD;
 else if(!strcmp(type, "mouse"))
  InputTypes[port] = PCEINPUT_MOUSE;
 else if(!strcmp(type, "tsushinkb"))
  InputTypes[port] = PCEINPUT_TSUSHINKB;
 else
  InputTypes[port] = PCEINPUT_NONE;

 data_ptr[port] = (uint8 *)ptr;

 RemakeDevices(port);
}

uint16 INPUT_HESHack(void)
{
 uint16 ret = 0;

 switch(InputTypes[0])
 {
// EMSCRIPTEN what kind of lunatic came up with this shit????			 
  case PCEINPUT_GAMEPAD: ret = MDFN_de16lsb(data_ptr[0]);
			 break;
 }
 return(ret);
}

void INPUT_Frame(void)
{
 for(int i = 0; i < 5; i++)
  if(devices[i])
   devices[i]->Update(data_ptr[i]);
}

void INPUT_AdjustTS(int32 delta_timestamp)
{
 for(int i = 0; i < 5; i++)
  if(devices[i])
   devices[i]->AdjustTS(delta_timestamp);
}

static INLINE uint8 RealPortRead(int32 timestamp, int which)
{
 uint8 ret = 0xF;

 //printf("RealInputRead: %d %d\n", which, timestamp);
 if(devices[which])
  ret = devices[which]->Read(timestamp);

 return(ret);
}

static INLINE void RealPortWrite(int32 timestamp, int which, bool old_SEL, bool new_SEL, bool old_CLR, bool new_CLR)
{
 if(devices[which])
  devices[which]->Write(timestamp, old_SEL, new_SEL, old_CLR, new_CLR);
}

static INLINE uint8 MultiTapRead(int32 timestamp)
{
 uint8 ret = 0xF;

 if(read_index > 4)
  ret = 0;
 else
  ret = RealPortRead(timestamp, read_index);

 return(ret);
}

static INLINE void MultiTapWrite(int32 timestamp, bool old_SEL, bool new_SEL, bool old_CLR, bool new_CLR)
{
 for(int i = 0; i < 5; i++)
  RealPortWrite(timestamp, i, old_SEL, new_SEL, old_CLR, new_CLR);

 // SEL held high, CLR transitions from 0->1, reset counter.
 // Scratch that, only check if SEL is high on the write that changes CLR from 0 to 1, to fix "Strip Fighter".
 if(/*old_SEL &&*/ new_SEL && !old_CLR && new_CLR)
 {
  //puts("Reset read index");

  read_index = 0;
 }
 // CLR held low, SEL transitions from 0->1, increment counter.
 else if(!old_CLR && !new_CLR && !old_SEL && new_SEL)
 {
  //puts("Increment read index");
  if(read_index < 255)
   read_index++;
 }
}

uint8 INPUT_Read(int32 timestamp, unsigned int A)
{
 uint8 ret;

 //{
 // extern VCE *vce;
 // printf("Input Read: %04x, %d\n", A, vce->GetScanlineNo());
 //}

 if(MultiTapEnabled && InputTypes[0] != PCEINPUT_TSUSHINKB)
  ret = MultiTapRead(timestamp);
 else
  ret = RealPortRead(timestamp, 0);

 //printf("Input read: %04x, %02x\n",A,ret);

 if(!PCE_IsCD)
  ret |= 0x80; // Set when CDROM is not attached

 //ret |= 0x40; // PC Engine if set, TG16 if clear.  Let's leave it clear, PC Engine games don't seem to mind if it's clear, but TG16 games barf if it's set.

 ret |= 0x30; // Always-set?

 return(ret);
}

void INPUT_Write(int32 timestamp, unsigned int A, uint8 V)
{
 //printf("Input Write: %04x, %02x\n", A, V);
 bool new_SEL = V & 0x1;
 bool new_CLR = V & 0x2;

 if(MultiTapEnabled)
  MultiTapWrite(timestamp, SEL, new_SEL, CLR, new_CLR);
 else
  RealPortWrite(timestamp, 0, SEL, new_SEL, CLR, new_CLR);

 SEL = new_SEL;
 CLR = new_CLR;
}

void PCEINPUT_Power(int32 timestamp)
{
 read_index = 0;
 SEL = 0;
 CLR = 0;

 for(int i = 0; i < 5; i++)
  if(devices[i])
   devices[i]->Power(timestamp);
}

int INPUT_StateAction(StateMem *sm, int load, int data_only)
{
 SFORMAT StateRegs[] =
 {
  SFVAR(SEL),
  SFVAR(CLR),
  SFVAR(read_index),
  SFEND
 };

 int ret = MDFNSS_StateAction(sm, load, data_only, StateRegs, "JOY");

 for(int i = 0; i < 5; i++)
 {
  if(devices[i])
  {
   char sn[8];
   trio_snprintf(sn, 8, "INP%d", i);
   ret &= devices[i]->StateAction(sm, load, data_only, sn);
  }
 }
 
 return(ret);
}

static const std::vector<InputDeviceInfoStruct> InputDeviceInfo =
{
 // None
 {
  "none",
  "none",
  NULL,
  IDII_Empty
 },

 // Gamepad
 {
  "gamepad",
  "Gamepad",
  NULL,
  PCE_GamepadIDII
 },

 // Mouse
 {
  "mouse",
  "Mouse",
  NULL,
  PCE_MouseIDII
 },
};

static const std::vector<InputDeviceInfoStruct> InputDeviceInfoPort1 =
{
 // None
 {
  "none",
  "none",
  NULL,
  IDII_Empty
 },

 // Gamepad
 {
  "gamepad",
  "Gamepad",
  NULL,
  PCE_GamepadIDII
 },

 // Mouse
 {
  "mouse",
  "Mouse",
  NULL,
  PCE_MouseIDII
 },

 // Tsushin Keyboard
 {
  "tsushinkb",
  "Tsushin Keyboard",
  NULL,
  PCE_TsushinKBIDII,
  InputDeviceInfoStruct::FLAG_KEYBOARD
 },
};

const std::vector<InputPortInfoStruct> PCEPortInfo =
{
 { "port1", "Port 1", InputDeviceInfoPort1, "gamepad" },
 { "port2", "Port 2", InputDeviceInfo, "gamepad" },
 { "port3", "Port 3", InputDeviceInfo, "gamepad" },
 { "port4", "Port 4", InputDeviceInfo, "gamepad" },
 { "port5", "Port 5", InputDeviceInfo, "gamepad" },
};

static void SyncSettings(void)
{
 MDFNGameInfo->mouse_sensitivity = 0.50; // MDFN_GetSettingF("pce.mouse_sensitivity"); EMSCRIPTEN
 MultiTapEnabled = 0; // MDFN_GetSettingB("pce.input.multitap"); EMSCRIPTEN
 DisableSR = 0; // MDFN_GetSettingB("pce.disable_softreset"); EMSCRIPTEN
}

};
