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

#include <mednafen/mednafen.h>
#include <mednafen/general.h>
#include <sys/types.h>
#include <trio/trio.h>
#include "cdromif.h"
#include "CDAccess.h"

using namespace CDUtility;

enum
{
 // Status/Error messages
 CDIF_MSG_DONE = 0,		// Read -> emu. args: No args.
 CDIF_MSG_INFO,			// Read -> emu. args: str_message
 CDIF_MSG_FATAL_ERROR,		// Read -> emu. args: *TODO ARGS*

 //
 // Command messages.
 //
 CDIF_MSG_DIEDIEDIE,		// Emu -> read

 CDIF_MSG_READ_SECTOR,		/* Emu -> read
					args[0] = lba
				*/
};

class CDIF_Message
{
 public:

 CDIF_Message();
 CDIF_Message(unsigned int message_, uint32 arg0 = 0, uint32 arg1 = 0, uint32 arg2 = 0, uint32 arg3 = 0);
 CDIF_Message(unsigned int message_, const std::string &str);
 ~CDIF_Message();

 unsigned int message;
 uint32 args[4];
 void *parg;
 std::string str_message;
};

class CDIF_Queue
{
 public:

 CDIF_Queue();
 ~CDIF_Queue();

 bool Read(CDIF_Message *message, bool blocking = true);

 void Write(const CDIF_Message &message);

 private:
 std::queue<CDIF_Message> ze_queue;
 /* EMSCRIPTEN
 MDFN_Mutex *ze_mutex;
 MDFN_Cond *ze_cond;
*/
};


typedef struct
{
 bool valid;
 bool error;
 int32 lba;
 uint8 data[2352 + 96];
} CDIF_Sector_Buffer;




// TODO: prohibit copy constructor
class CDIF_ST : public CDIF
{
 public:

 CDIF_ST(std::unique_ptr<CDAccess> cda);
 virtual ~CDIF_ST();

 virtual void HintReadSector(int32 lba);
 virtual bool ReadRawSector(uint8 *buf, int32 lba);
 virtual bool ReadRawSectorPWOnly(uint8* pwbuf, int32 lba, bool hint_fullread);

 private:
 std::unique_ptr<CDAccess> disc_cdaccess;
};

CDIF::CDIF() : UnrecoverableError(false)
{

}

CDIF::~CDIF()
{

}


CDIF_Message::CDIF_Message()
{
 message = 0;

 memset(args, 0, sizeof(args));
}

CDIF_Message::CDIF_Message(unsigned int message_, uint32 arg0, uint32 arg1, uint32 arg2, uint32 arg3)
{
 message = message_;
 args[0] = arg0;
 args[1] = arg1;
 args[2] = arg2;
 args[3] = arg3;
}

CDIF_Message::CDIF_Message(unsigned int message_, const std::string &str)
{
 message = message_;
 str_message = str;
}

CDIF_Message::~CDIF_Message()
{

}

CDIF_Queue::CDIF_Queue()
{
// ze_mutex = MDFND_CreateMutex();
// ze_cond = MDFND_CreateCond();
}

CDIF_Queue::~CDIF_Queue()
{
// MDFND_DestroyMutex(ze_mutex);
// MDFND_DestroyCond(ze_cond);
}

// Returns false if message not read, true if it was read.  Will always return true if "blocking" is set.
// Will throw MDFN_Error if the read message code is CDIF_MSG_FATAL_ERROR
bool CDIF_Queue::Read(CDIF_Message *message, bool blocking)
{
 bool ret = true;
/*
 //
 //
 //
 MDFND_LockMutex(ze_mutex);

 if(blocking)
 {
  while(ze_queue.size() == 0)	// while, not just if.
  {
 //  MDFND_WaitCond(ze_cond, ze_mutex);
  }
 }

 if(ze_queue.size() == 0)
  ret = false;
 else
 {
  *message = ze_queue.front();
  ze_queue.pop();
 }  

 MDFND_UnlockMutex(ze_mutex);
 //
 //
 //
*/
 if(ret && message->message == CDIF_MSG_FATAL_ERROR)
  throw MDFN_Error(0, "%s", message->str_message.c_str());

 return(ret);
}

void CDIF_Queue::Write(const CDIF_Message &message)
{
	/*
 MDFND_LockMutex(ze_mutex);

 try
 {
  ze_queue.push(message);
 }
 catch(...)
 {
  fprintf(stderr, "\n\nCDIF_Message queue push failed!!!  (We now return you to your regularly unscheduled lockup)\n\n");
 }

 MDFND_SignalCond(ze_cond);	// Signal while the mutex is held to prevent icky race conditions.

 MDFND_UnlockMutex(ze_mutex);
 */
}


bool CDIF::ValidateRawSector(uint8 *buf)
{
 int mode = buf[12 + 3];

 if(mode != 0x1 && mode != 0x2)
  return(false);

 if(!edc_lec_check_and_correct(buf, mode == 2))
  return(false);

 return(true);
}

int CDIF::ReadSector(uint8* buf, int32 lba, uint32 sector_count)
{
 fprintf(stderr, "ERROR: CDIF::ReadSector\n");
 return 0;
}

//
//
// Single-threaded implementation follows.
//
//

CDIF_ST::CDIF_ST(std::unique_ptr<CDAccess> cda) : disc_cdaccess(std::move(cda))
{
 fprintf(stderr, "ERROR: CDIF_ST::CDIF_ST\n");
}

CDIF_ST::~CDIF_ST()
{

}

void CDIF_ST::HintReadSector(int32 lba)
{
 // TODO: disc_cdaccess seek hint? (probably not, would require asynchronousitycamel)
}

bool CDIF_ST::ReadRawSector(uint8 *buf, int32 lba)
{
 fprintf(stderr, "ERROR: CDIF_ST::ReadRawSector\n");
 return false;
}

bool CDIF_ST::ReadRawSectorPWOnly(uint8* pwbuf, int32 lba, bool hint_fullread)
{
 fprintf(stderr, "ERROR: CDIF_ST::ReadRawSector\n");
 return false;
}

class CDIF_Stream_Thing : public Stream
{
 public:

 CDIF_Stream_Thing(CDIF *cdintf_arg, uint32 lba_arg, uint32 sector_count_arg);
 ~CDIF_Stream_Thing();

 virtual uint64 attributes(void) override;
  
 virtual uint64 read(void *data, uint64 count, bool error_on_eos = true) override;
 virtual void write(const void *data, uint64 count) override;
 virtual void truncate(uint64 length) override;

 virtual void seek(int64 offset, int whence) override;
 virtual uint64 tell(void) override;
 virtual uint64 size(void) override;
 virtual void flush(void) override;
 virtual void close(void) override;

 private:
 CDIF *cdintf;
 const uint32 start_lba;
 const uint32 sector_count;
 int64 position;
};

CDIF_Stream_Thing::CDIF_Stream_Thing(CDIF *cdintf_arg, uint32 start_lba_arg, uint32 sector_count_arg) : cdintf(cdintf_arg), start_lba(start_lba_arg), sector_count(sector_count_arg)
{

}

CDIF_Stream_Thing::~CDIF_Stream_Thing()
{

}

uint64 CDIF_Stream_Thing::attributes(void)
{
 return(ATTRIBUTE_READABLE | ATTRIBUTE_SEEKABLE);
}
  
uint64 CDIF_Stream_Thing::read(void *data, uint64 count, bool error_on_eos)
{
 if(count > (((uint64)sector_count * 2048) - position))
 {
  if(error_on_eos)
  {
   throw MDFN_Error(0, "EOF");
  }

  count = ((uint64)sector_count * 2048) - position;
 }

 if(!count)
  return(0);

 for(uint64 rp = position; rp < (position + count); rp = (rp &~ 2047) + 2048)
 {
  uint8 buf[2048];  

  if(!cdintf->ReadSector(buf, start_lba + (rp / 2048), 1))
  {
   throw MDFN_Error(ErrnoHolder(EIO));
  }
  
  //::printf("Meow: %08llx -- %08llx\n", count, (rp - position) + std::min<uint64>(2048 - (rp & 2047), count - (rp - position)));
  memcpy((uint8*)data + (rp - position), buf + (rp & 2047), std::min<uint64>(2048 - (rp & 2047), count - (rp - position)));
 }

 position += count;

 return count;
}

void CDIF_Stream_Thing::write(const void *data, uint64 count)
{
 throw MDFN_Error(ErrnoHolder(EBADF));
}

void CDIF_Stream_Thing::truncate(uint64 length)
{
 throw MDFN_Error(ErrnoHolder(EBADF));
}

void CDIF_Stream_Thing::seek(int64 offset, int whence)
{
 int64 new_position;

 switch(whence)
 {
  default:
	throw MDFN_Error(ErrnoHolder(EINVAL));
	break;

  case SEEK_SET:
	new_position = offset;
	break;

  case SEEK_CUR:
	new_position = position + offset;
	break;

  case SEEK_END:
	new_position = ((int64)sector_count * 2048) + offset;
	break;
 }

 if(new_position < 0 || new_position > ((int64)sector_count * 2048))
  throw MDFN_Error(ErrnoHolder(EINVAL));

 position = new_position;
}

uint64 CDIF_Stream_Thing::tell(void)
{
 return position;
}

uint64 CDIF_Stream_Thing::size(void)
{
 return(sector_count * 2048);
}

void CDIF_Stream_Thing::flush(void)
{

}

void CDIF_Stream_Thing::close(void)
{

}


Stream *CDIF::MakeStream(int32 lba, uint32 sector_count)
{
 return new CDIF_Stream_Thing(this, lba, sector_count);
}


CDIF *CDIF_Open(const std::string& path, bool image_memcache)
{
 std::unique_ptr<CDAccess> cda(CDAccess_Open(path, image_memcache));

// if(!image_memcache)
//  return new CDIF_MT(std::move(cda));
// else
  return new CDIF_ST(std::move(cda));
}
