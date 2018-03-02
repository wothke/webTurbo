::  POOR MAN'S DOS PROMPT BUILD SCRIPT.. make sure to delete the respective built/*.bc files before building!
::  existing *.bc files will not be recompiled. 

setlocal enabledelayedexpansion

SET ERRORLEVEL
VERIFY > NUL

:: **** use the "-s WASM" switch to compile WebAssembly output. warning: the SINGLE_FILE approach does NOT currently work in Chrome 63.. ****
set "OPT=  -s WASM=0 -DLSB_FIRST -DPSS_STYLE=1 -DSIZEOF_DOUBLE=8 -s ASSERTIONS=2 -s FORCE_FILESYSTEM=1 -DARCH_X86_32 -DHAVE_LIBTRIO -DHAVE_LIMITS_H -Wcast-align -fno-strict-aliasing -s VERBOSE=0 -s SAFE_HEAP=0 -s DISABLE_EXCEPTION_CATCHING=0  -DEMU_COMPILE -DEMU_LITTLE_ENDIAN -DHAVE_STDINT_H -DNO_DEBUG_LOGS -Wno-pointer-sign -I. -I.. -I../src -I../zlib  -Os -O3 "

:: HES does not seem to use zlib - so save some space (see -DUSE_NULL_ZLIB)
::if not exist "built/zlib.bc" (
::	call emcc.bat %OPT% ../zlib/adler32.c ../zlib/compress.c ../zlib/crc32.c ../zlib/gzio.c ../zlib/uncompr.c ../zlib/deflate.c ../zlib/trees.c ../zlib/zutil.c ../zlib/inflate.c ../zlib/infback.c ../zlib/inftrees.c ../zlib/inffast.c  -o built/zlib.bc
::	IF !ERRORLEVEL! NEQ 0 goto :END
::)

if not exist "built/3dParty.bc" (
	call emcc.bat %OPT% ../src/trio/trio.c ../src/trio/triostr.c ../src/trio/trionan.c -o built/3dParty.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)
if not exist "built/pce.bc" (
	call emcc.bat -std=c++11 %OPT% ../src/pce/pcecd_null.cpp ../src/pce/huc6280.cpp ../src/pce/pce.cpp ../src/pce/vce.cpp ../src/pce/input.cpp ../src/pce/huc.cpp ../src/pce/hes.cpp ../src/pce/tsushin.cpp ../src/pce/mcgenjin.cpp ../src/pce/input/gamepad.cpp ../src/pce/input/tsushinkb.cpp ../src/pce/input/mouse.cpp -o built/pce.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

:: ../src/mednafen/cdrom/scsicd.cpp
if not exist "built/mednafen.bc" (
	call emcc.bat -std=c++11 %OPT% ../src/mednafen/mempatcher.cpp  ../src/mednafen/sound/okiadpcm.cpp ../src/mednafen/error.cpp ../src/mednafen/endian.cpp ../src/mednafen/file.cpp ../src/mednafen/FileStream.cpp ../src/mednafen/general.cpp ../src/mednafen/git.cpp ../src/mednafen/Time.cpp ../src/mednafen/state.cpp ../src/mednafen/Stream.cpp ../src/mednafen/MemoryStream.cpp  ../src/mednafen/string/escape.cpp ../src/mednafen/string/string.cpp ../src/mednafen/sound/OwlResampler.cpp ../src/mednafen/sound/Fir_Resampler.cpp ../src/mednafen/hw_video/huc6270/vdc.cpp  ../src/mednafen/cdrom/CDAccess.cpp ../src/mednafen/cdrom/cdromif.cpp  ../src/mednafen/cdrom/CDUtility.cpp ../src/mednafen/hw_sound/pce_psg/pce_psg.cpp ../src/mednafen/hash/md5.cpp ../src/mednafen/compress/GZFileStream.cpp -o built/mednafen.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

call emcc.bat -std=c++11 %OPT% -DUSE_NULL_ZLIB -s TOTAL_MEMORY=134217728 --memory-init-file 0 --closure 1 --llvm-lto 1  built/3dParty.bc built/pce.bc built/mednafen.bc  mednafen_light.cpp adapter.cpp   -s EXPORTED_FUNCTIONS="['_emu_setup', '_emu_init','_emu_teardown','_emu_get_current_position','_emu_seek_position','_emu_get_max_position','_emu_set_subsong','_emu_get_track_info','_emu_get_sample_rate','_emu_get_audio_buffer','_emu_get_audio_buffer_length','_emu_compute_audio_samples', '_malloc', '_free']"  -o htdocs/hes.js  -s SINGLE_FILE=0 -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'Pointer_stringify']"  -s BINARYEN_ASYNC_COMPILATION=1 -s BINARYEN_TRAP_MODE='clamp' && copy /b shell-pre.js + htdocs\hes.js + shell-post.js htdocs\web_hes3.js && del htdocs\hes.js && copy /b htdocs\web_hes3.js + hes_adapter.js htdocs\backend_hes.js && del htdocs\web_hes3.js
:END