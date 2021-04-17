/*
* This file adapts "mednafen/PCE" to the interface expected by my generic JavaScript player..
*
* Copyright (C) 2018 Juergen Wothke
*
* LICENSE
* 
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2.1 of the License, or (at
* your option) any later version. This library is distributed in the hope
* that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/

#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>     

#include <iostream>
#include <fstream>

#include <mednafen/mednafen.h>
#include <mednafen/git.h>
#include <pce/hes.h>


#ifdef EMSCRIPTEN
#define EMSCRIPTEN_KEEPALIVE __attribute__((used))
#else
#define EMSCRIPTEN_KEEPALIVE
#endif


// see Sound::Sample::CHANNELS
#define CHANNELS 2				
#define BYTES_PER_SAMPLE 2
#define SAMPLE_BUF_SIZE	1024

// mednafen: "Guaranteed to be at least 500ms in length, but emulation code really 
// shouldn't exceed 40ms or so.  Additionally, if emulation code"
int16_t sample_buffer[SAMPLE_BUF_SIZE * CHANNELS];	// WTF? 44100/2 *2bytes -> just in case?
int samples_available= 0;

const char* info_texts[2];

#define TEXT_MAX	255
char title_str[TEXT_MAX];
char track_str[TEXT_MAX];

EmulateSpecStruct espec;

// some debug output needed?
void MDFN_indent(int indent) {}
void MDFN_printf(const char *format, ...) noexcept {}
void MDFN_Notify(MDFN_NoticeType t, const char* format, ...) noexcept {}
void MDFN_DebugPrintReal(const char *file, const int line, const char *format, ...) {}

void MDFND_OutputInfo(const char *s) noexcept {
//	fprintf(stderr, s);
}
void MDFND_OutputNotice(MDFN_NoticeType t, const char* s) noexcept {
	MDFND_OutputInfo(s);
}

// dummy impl for stuff that is irrelecant for HES
void MDFND_MidSync(const EmulateSpecStruct *espec) {}
void MDFN_DoSimpleCommand(int cmd) {}
void MDFN_QSimpleCommand(int cmd){}
void MDFNI_Power(void) {}
void MDFNI_Reset(void) {}
void MDFNI_ToggleDIPView(void) {}
void MDFNI_ToggleDIP(int which) {}
void MDFNI_InsertCoin(void) {}

uint8* MDFNI_SetInput(const uint32 port, const uint32 type) { return NULL;}
void MDFNI_SetLayerEnableMask(uint64 mask) {}
bool MDFNI_SetMedia(uint32 drive_idx, uint32 state_idx, uint32 media_idx, uint32 orientation_idx) { return false; }

bool MDFN_UntrustedSetMedia(uint32 drive_idx, uint32 state_idx, uint32 media_idx, uint32 orientation_idx){ return false; }
void MDFN_MediaSetNotification(uint32 drive_idx, uint32 state_idx, uint32 media_idx, uint32 orientation_idx) {}
void MDFN_StateAction(StateMem *sm, const unsigned load, const bool data_only) {}
int MDFNI_LoadSettings(const char* path){ return 1;}
void MDFNI_Kill(void){}
void MDFNI_DumpModulesDef(const char *fn){}

// it does not seem that the zlib stuff is actually used in the .hes files scenario..
// but if I am wrong just remove the -USE_NULL_ZLIB from the make script
#ifdef USE_NULL_ZLIB
#define gzFile void*
#define voidp void*
#ifndef z_off_t
#  define z_off_t long
#endif
#ifndef OF /* function prototypes */
#  ifdef STDC
#    define OF(args)  args
#  else
#    define OF(args)  ()
#  endif
#endif
extern "C" {
	int     gzclose OF((gzFile file)) 		{ return 0;}
	gzFile  gzopen  OF((const char *path, const char *mode))	{ return NULL;}
	int  gzeof OF((gzFile file))		{ return 0;}
	const char *  gzerror OF((gzFile file, int *errnum)) { return NULL;}
	int     gzflush OF((gzFile file, int flush))		{ return 0;}
	int     gzgetc OF((gzFile file))		{ return 0;}
	int     gzread  OF((gzFile file, voidp buf, unsigned len))		{ return 0;}
	z_off_t     gzseek OF((gzFile file,z_off_t offset, int whence))		{ return 0;}
	z_off_t     gztell OF((gzFile file))		{ return 0;}
	int     gzwrite OF((gzFile file, voidpc buf, unsigned len))		{ return 0;}
	gzFile	gzdopen  OF((int fd, const char *mode))		{ return NULL;}
}
#endif

static struct
{
 std::unique_ptr<MDFN_Surface> surface = nullptr;
 MDFN_Rect rect;
 std::unique_ptr<int32[]> lw = nullptr;
 int field = -1;
} SoftFB[2];

static bool SoftFB_BackBuffer = false;


void initEmuStruct() {
	// below settings may not be the required minimum.. but they work and I did not waste time to check..
	int i= 0;
	uint32 pitch32 = 1365; 
	MDFN_PixelFormat nf(MDFN_COLORSPACE_RGB, 0, 8, 16, 24);
	SoftFB[i].surface.reset(new MDFN_Surface(NULL, 1365, 270, pitch32, nf));	// 1365: Framebuffer width, 270: Framebuffer height
	  SoftFB[i].lw.reset(new int32[270]);

	SoftFB[i].surface->Fill(0, 0, 0, 0);

	SoftFB[i].rect.w = std::min<int32>(16, SoftFB[i].surface->w);
	SoftFB[i].rect.h = std::min<int32>(16, SoftFB[i].surface->h);
	SoftFB[i].lw[0] = ~0;	
	SoftFB[SoftFB_BackBuffer].field = -1;;
	
	memset(&espec, 0, sizeof(EmulateSpecStruct));
	
	espec.surface = SoftFB[SoftFB_BackBuffer].surface.get();
	espec.LineWidths = SoftFB[SoftFB_BackBuffer].lw.get();
	espec.skip = 0;;
	espec.soundmultiplier = 1;;
	espec.NeedRewind = 0;;
	
	espec.SoundRate = 44100;
	espec.SoundBuf = sample_buffer;
	espec.SoundBufMaxSize = SAMPLE_BUF_SIZE * CHANNELS;
	espec.SoundVolume = 1;
}

// "player callbacks" used by the emulator's HES_Update call
void Player_Init(int tsongs, const std::string &album, const std::string &artist, 
	const std::string &copyright, const std::vector<std::string> &snames = std::vector<std::string>(), 
	bool override_gi = true)
{
	// note: the info passed here seems to be pretty useless.. tsongs is always 256 and the remaining infos are empty
	// judging by the output of other players the "rom name" seems to be the most useful thing to display..
}

void Player_Draw(MDFN_Surface *surface, MDFN_Rect *dr, int track, int16 *samples, int32 sampcount) {	
	if (samples != sample_buffer)  {
		fprintf(stderr, "ERRROR: invalid assumption\n");
	} else {
		samples_available= sampcount;		
	}
}

struct StaticBlock {
    StaticBlock(){
		info_texts[0]= title_str;
		info_texts[1]= track_str;
    }
};
	
void meta_clear() {
	snprintf(title_str, TEXT_MAX, "");
	snprintf(track_str, TEXT_MAX, "");
}

const std::vector<MDFNSetting> driverSettings;

static StaticBlock g_emscripen_info;

extern "C" void emu_teardown (void)  __attribute__((noinline));
extern "C" void EMSCRIPTEN_KEEPALIVE emu_teardown (void) {
}

extern "C" int emu_setup(char *unused) __attribute__((noinline));	// FIXME not externally exposed !!!
extern "C" EMSCRIPTEN_KEEPALIVE int emu_setup(char *unused)
{
	initEmuStruct();
	
	// Call this function as early as possible, even before MDFNI_Initialize()
	if (MDFNI_InitializeModules()) {
		if (MDFNI_Initialize("", driverSettings)) {	// FIXME check it this works without config files
			if ( MDFNI_LoadSettings("") != 0) {	// unnecessary
				return 1;
			}		
		}
	}
	return 0;
}

extern "C" int emu_init(char *basedir, char *songmodule) __attribute__((noinline));
extern "C" EMSCRIPTEN_KEEPALIVE int emu_init(char *basedir, char *songmodule)
{
	meta_clear();
	emu_teardown();

	if (!emu_setup(NULL)) return 1;
	
	std::string file= std::string(basedir)+"/"+std::string(songmodule);
	
	// Returns NULL on error.
	MDFNGI *tmp= MDFNI_LoadGame(NULL, file.c_str(), false);
	if(tmp != NULL) {
		// poor man's song name
		
		std::string displayname= std::string(songmodule);
		displayname.erase( displayname.find_last_of( '.' ) );	// remove ext

		size_t n=  displayname.find_last_of ('/');	// remove path
		displayname= displayname.substr((n<displayname.length())? n : 0);
		
		snprintf(title_str, TEXT_MAX, "%s", displayname.c_str());

		uint8 track= MDFN_IEN_PCE::HES_GetTrack();	// read default
		snprintf(track_str, TEXT_MAX, "%d", track);
		
		return 0;
	 }
	return 1;
}

extern "C" int emu_get_sample_rate() __attribute__((noinline));
extern "C" EMSCRIPTEN_KEEPALIVE int emu_get_sample_rate()
{
	return espec.SoundRate;
}

extern void MDFN_IEN_PCE::HES_SetTrack(uint8 track);
extern uint8 MDFN_IEN_PCE::HES_GetTrack();
 
extern "C" int emu_set_subsong(int subsong, unsigned char boost) __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_set_subsong(int subsong, unsigned char boost) {
	/* .hes files to contain multiple tracks but used IDs are not obvious:
	HES files are garbage: there are track-ids but within the range of 0-255
	used track-ids may be hidden anywhere. The "default" track-id
	defined for a song sometimes points to nothing so that drag&drop does not
	work to play anything audible.
	*/	
	
	if (subsong > 0) {
		MDFN_IEN_PCE::HES_SetTrack((uint8)subsong-1);
	}
	//uint8 track= MDFN_IEN_PCE::HES_GetTrack();	// also read default when not set explicitly
	//snprintf(track_str, TEXT_MAX, "%d", track);
	
	return 0;
}

extern "C" const char** emu_get_track_info() __attribute__((noinline));
extern "C" const char** EMSCRIPTEN_KEEPALIVE emu_get_track_info() {
	return info_texts;
}

extern "C" char* EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer(void) __attribute__((noinline));
extern "C" char* EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer(void) {
	return (char*)sample_buffer;
}

extern "C" long EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer_length(void) __attribute__((noinline));
extern "C" long EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer_length(void) {
	return samples_available;
}

extern "C" int emu_compute_audio_samples() __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_compute_audio_samples() {
	/* Emulates a frame. */
	MDFNI_Emulate(&espec);	// triggers "Player_Draw"
	
	return !(samples_available > 0);
}

extern "C" int emu_get_current_position() __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_get_current_position() {
	return -1;		// not supported
}

extern "C" void emu_seek_position(int pos) __attribute__((noinline));
extern "C" void EMSCRIPTEN_KEEPALIVE emu_seek_position(int sampleTime) {
	// not supported
}

extern "C" int emu_get_max_position() __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_get_max_position() {
	return -1;	// not supported
}

