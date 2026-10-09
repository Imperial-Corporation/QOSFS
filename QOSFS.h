#pragma once

/*
 This project is a prototype of QOSFS
 It is not meant to be used in actual projects

 It contains many code parts that are not tested and may not even be compilable.

 QOSFS V0.1
 Quick
 Optimized
 Simple
 File
 System
 -> Q.O.S.F.S.
 */

/*
 Function required.
 QOSFS requires the following functions:
 ---- Disk I/O functions ----
 DISKRES disk_read  (I8 prdv, I8 *buff, BLOCK_ADDR sector, UINT count);
 DISKRES disk_write (I8 pdrv, const I8* buff, BLOCK_ADDR sector, UINT count);
 DISKRES disk_ioctl (I8 pdrv, I8 cmd, void* buff);

 ---- String manipulation functions ----
 Basic standard C functions for string manipulation such as:
 strcat
 strcpy
 strcmp
 itoa/atoi
 strlen

 Are required.
 */

#include "QOSFS_conf.h"

#define FLAG_FRAGMENTED   0
#define FLAG_HIDDEN       1
#define FLAG_TRASHED      2
#define FLAG_READONLY     3
#define FLAG_EXECUTABLE   4
#define FLAG_UNTOUCHABLE  5
#define FLAG_EXTENDED     6

// Include your own libraries there
#include "../primordial.h"
#include "../diskio.h"

// ---

extern const char QOSFS_VERSION[11];
extern const char QOSFS_DESCRIPTION[256];

typedef enum {
    QOS_OK = 0,     //0 Ok
    QOS_DISKERR,    //1 diskio error
    QOS_UNKERR,     //2 Unknown error
    QOS_FATAL,      //3 FATAL ERROR. QOSFS will shutdown partially and won't be able to do anything unless he error gets fixed. The error details can be get by calling GetErrorInfo(I8* buf)

    QOS_NOFS,       //4 No filesystem found
    QOS_NOSPACE,    //5 Not enough free space found
    QOS_NOMEM,      //6 Not enough free memory found (RAM)
    QOS_NOUSE,      //7 The function called was unuseful, whether because of its parameters, or something else that made the function not worth of calling except loosing some CPU cycles. Take this return value as a warning, if the function returns this most of the time maybe your code is calling it too often or at the wrong moment.

    QOS_INVARG,     //8 Invalid argument
    QOS_INVFIL,     //9 Invalid file
    QOS_INVFOL,     //10 Invalid folder
    QOS_INVSS,      //11 Invalid sector size (not 512)
    QOS_INVNAME,    //12 Invalid name

    QOS_TESTERR     //13 An internal test (often asked by a function parameter) returned an error.
} COMRES;

typedef unsigned int	UINT;	/* int must be 16-bit or 32-bit */
typedef uint8_t	        I8;	    /* char must be 8-bit */
typedef uint16_t		I16;	/* 16-bit unsigned */
typedef uint32_t		I32;	/* 32-bit unsigned */
typedef uint64_t		I64;	/* 64-bit unsigned */

/*
 These three extern functions are meant to be given by the user of the filesystem.
 The main reason of it is that QOSFS can't (and is not meant to) know wich type of disk it is on,
 wich driver to use, wich device it is on and where on the disk it has to effectue these actions.

 Adding all of that in QOSFS would make it heavy and not even sure if it would work everywhere.

 Note: QOSFS is also made to be compatible with Imperial NextOS, so it will
 mainly act like FatFs and be very similar to it on the interface side.
 */

/*
 Notes about how QOSFS will sort files and folder on the disk.

 Every file will be adressed to its folder.
 and folders will be adressed to their files.

 So QOSFS doesn't have to search; it always knows where to find the stuff you ask.

 For optimization purposes, QOSFS will accept a max amount of 256 files per folder.
 This parameter can be changed in QOSFS_conf.h

 QOSFS will have a "folder_open" or similar function.
 This function loads in memory a folder so QOSFS can access its file's addresses.
 If you try acting on a file in a non-opened folder, QOSFS will automatically open it, BUT
 QOSFS NEVER closes a folder using the "folder_close" function!
 QOSFS has a max amount of opened folders at the same time, it can also be configured in QOSFS_conf.h
 By default it is 80, you can add more if you're using a big system or less if you need to be as lightweight as possible.
 I don't recommand to put it a less than 10 if you have multitasking. It would make your system extremely slow and maybe even crash it.

 QOSFS uses mainly addresses for finding files and what it will call "zones" to sort files and folders.
 Metadata will be the main tool of the FS.
 For example, a folder will have as Metadata its children adresses, size, name, and all the other stuff
 Same for files.
 */

typedef struct {
    I64 addr;
    I64 count;
} frzn;

extern frzn FREEZONES[1024];

extern I64 DISK_SIZE;

extern I64 EOD;

COMRES QOSFS_init_globals();

COMRES QOSFS_init();

typedef struct {
    char version[10];
    char desc[256];
    I64  EOD;
    I8   flags;

    I8   reserved[241];
} QOSFS_S0;

COMRES QOSFS_format(I64 start, I64 end, I8 volume, _Bool testparam);

I64 find_free_zone(I64 size);

typedef struct {
    I64 addr;
    I64 size;
} extent;

typedef struct {
    // Name
    char name[129];
    // Size
    I64 size;
    // 1 byte flags
    I8 flags;
    // Number of extents if any
    I32 extent_count;
    // Parent folder address
    I64 parentfoladdr;
    // Years, months, days, hours, minutes and seconds for creating (0), modifying (1) and accessing (2)
    I16 years[3];
    I8 months[3];
    I8 days[3];
    I8 hours[3];
    I8 mins[3];
    I8 secs[3];
    extent extdata[15];
    char restant[90];
} FILH;

_Static_assert(sizeof(I8)  == 1, "I8 is not 8-bit");
_Static_assert(sizeof(I16) == 2, "I16 is not 16-bit");
_Static_assert(sizeof(I32) == 4, "I32 is not 32-bit");
_Static_assert(sizeof(I64) == 8, "I64 is not 64-bit");
_Static_assert(sizeof(extent) == 16,
               "extent must be exactly 16 bytes");
_Static_assert(sizeof(FILH) == 512,
               "FILH must be exactly 512 bytes");


COMRES QOSFS_fmake(const char* name, _Bool openparam);

COMRES QOSFS_fwrite(const char* path, const char* text);

COMRES QOSFS_fclose(const char* path);


/*
 Oh, I didn't take the time to talk about sector size.
 Well, QOSFS does not care at all the sector size, it is YOUR diskio interface that should handle that.
 QOSFS will just ask for "sector 4" and your diskio will multiply 4 by 512 if the sector size is 512 for example.
 */
