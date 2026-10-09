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
#include "../primordial.h"
#include "../diskio.h"

extern const char* QOSFS_VERSION;

typedef unsigned short DISKRES;

typedef enum {
    QOS_OK,
    QOS_DISKERR,
    QOS_NOFS,
    QOS_INVARG,
    QOS_INVFIL,
    QOS_INVFOL,
    QOS_UNKERR,
    QOS_NOSPACE
} COMRES;

typedef unsigned int	UINT;	/* int must be 16-bit or 32-bit */
typedef unsigned char	I8;	    /* char must be 8-bit */
typedef uint16_t		I16;	/* 16-bit unsigned */
typedef uint32_t		I32;	/* 32-bit unsigned */
typedef uint64_t		I64;	/* 64-bit unsigned */

typedef I64 BLOCK_ADDR;
typedef I64 ZONE_ADDR;

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

// Files
typedef struct {
    char* sign;
    char* name;
    BLOCK_ADDR start;
    BLOCK_ADDR end;

    BLOCK_ADDR folder;   //<- Folder
} filev1;

// Folders
typedef struct {
    BLOCK_ADDR files[MAX_FOLDER_FILE_COUNT];
} folderv1;

#define FILE_MTDT_BASE_SIZE 6

/*
 Lets say a zone takes 8192B.
 This way, QOSFS can address over 32TB of space.
 But, reading 8kB everytime even for files smaller than 2kB is... not optimized.
 So, a zone will be separated into blocks of 128B.
 This way, a 128GB HDD disk is separated into 16777216 zones,
 and a zone is made of 64 blocks, so 8192 / 64 = 128B by block.
 So QOSFS can be 32 bits-compatible, supporting 32TB disks and still being optimized.

 */

typedef struct {
    BLOCK_ADDR addr;
    BLOCK_ADDR count;
} frzn;

extern frzn FREEZONES[1024];

extern I64 DISK_SIZE;

extern I64 EOD;

COMRES QOSFS_init_globals();

/* QOSFS starts at LBA 2048
 From 2048, the 20 first blocks are for QOSFS data.
 Reminder: every block is 128B large, so 20 blocks are 2.56KB
 These 2.56KB are not actually needed, but it won't hurt anyone so
 it is just in case QOSFS needs 2560B for some reason.
 */

#define BLOCK_SIZE 128
#define ZONE_SIZE 64 * BLOCK_SIZE

COMRES QOSFS_init();

COMRES QOSFS_format(ZONE_ADDR start, ZONE_ADDR end);

BLOCK_ADDR find_free_zone(I64 size);

COMRES makefile(const char* name);
