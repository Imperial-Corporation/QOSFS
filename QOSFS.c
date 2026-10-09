#include "QOSFS.h"

/*
 Notes about bit manipulating because I don't wanna mess up with these:
 bits 0 to 7
 value |= (0/1 << n);
 Set bit

 value &= ~(0/1 << n);
 Clear bit

 value & (0/1 << n)
 Check bit

 value ^= (0/1 << n);
 Invert bit
 */

const char QOSFS_VERSION[11] = "QOSFS v0.1";
const char QOSFS_DESCRIPTION[256] = "First ever version of QOSFS. It introduces initialization, file creating, writing, reading and deleting. It can format a volume to install itself and already contains structures for future features.";

frzn FREEZONES[1024];

I64 DISK_SIZE;

I64 EOD;

I32 SSD;
/*
 Nah, that is not about SSD volumes.
 Sector Size Divisor
 That is the meaning...
 */

I64 x(I32 arg){
    return arg * SSD;
}

COMRES QOSFS_init_globals(){
    for (I16 i = 0; i >= 1023; ++i){
        FREEZONES[i].addr = 0;
        FREEZONES[i].count = 0;
    }
    I32 ssr;
    disk_ioctl(0, 2, &ssr);
    SSD = 1;
    if (ssr != 512){
        I32 ssr2 = ssr;
        while (ssr2 < 8192){
            SSD += 1;
            ssr2 *= SSD;
            if (ssr2 == ssr) break;
            // 1024, 2048, 4096 or 8192
            // Less than 512 is too small and more than 8192 is too big
        }
    }

    I64 seccount;
    disk_ioctl(0, 1, &seccount);

    DISK_SIZE = (I64)seccount * (I64)ssr;
}

COMRES QOSFS_init(){
    char* buf = "";
    disk_read(0, buf, 0, 1);
    if (!buf) return QOS_DISKERR;

    if (strncmp(buf, "QOSFS", 5))
        return QOS_NOFS;

    return QOS_OK;
}

I8 read_u8_le(const I8 *buf){
    return buf[0];
}

I16 read_u16_le(const I8 *buf){
    return
        ((I16)buf[0] << 0)  |
        ((I16)buf[1] << 8);
}

I32 read_u32_le(const I8 *buf){
    return
        ((I32)buf[0] << 0)  |
        ((I32)buf[1] << 8)  |
        ((I32)buf[2] << 16) |
        ((I32)buf[3] << 24);
}

I64 read_u64_le(const I8 *buf){
    return
        ((I64)buf[0] << 0)  |
        ((I64)buf[1] << 8)  |
        ((I64)buf[2] << 16) |
        ((I64)buf[3] << 24) |
        ((I64)buf[4] << 32) |
        ((I64)buf[5] << 40) |
        ((I64)buf[6] << 48) |
        ((I64)buf[7] << 56);
}

void write_u8_le(I8 *buf, I8 value){
    buf[0] = value;
}

void write_u16_le(I8 *buf, I16 value){
    buf[0] = value & 0xFF;
    buf[1] = (value >> 8) & 0xFF;
}

void write_u32_le(I8 *buf, I32 value){
    buf[0] = value & 0xFF;
    buf[1] = (value >> 8) & 0xFF;
    buf[2] = (value >> 16) & 0xFF;
    buf[3] = (value >> 24) & 0xFF;
}

void write_u64_le(I8 *buf, I64 value){
    buf[0] = (value >> 0)  & 0xFF;
    buf[1] = (value >> 8)  & 0xFF;
    buf[2] = (value >> 16) & 0xFF;
    buf[3] = (value >> 24) & 0xFF;
    buf[4] = (value >> 32) & 0xFF;
    buf[5] = (value >> 40) & 0xFF;
    buf[6] = (value >> 48) & 0xFF;
    buf[7] = (value >> 56) & 0xFF;
}

char CURRENT_PATH[512];

COMRES QOSFS_format(I64 start, I64 end, I8 volume, _Bool testparam){
    I8 VOID[512] = {0};

    if (start > end)
        return QOS_INVARG;

    if (start == end)
        return QOS_NOUSE;

    DRESULT res;
    while (end != start){
        res = disk_write(volume, VOID, end, 1);
        if (res != RES_OK) return QOS_DISKERR;
        --end;
    }
    /*
     QOSFS header -- sector 0:
     QOSFS vX.X
     QOSFS version description (256 characters, if less long than 256 it will still reserve 256)
     The version and description will be detailed as future versions will see it and
     adapt to the older structures if they ever change. There will also be a update
     function that updates the filesystem without deleting or breaking anything,
     but it will be added in future versions too.
     EOD
     flags:
     trash full?      0/1   1   Is the trash full (128 objects)?
     For the moment there's only 1 flag but I will probably add parameters or other flags later.
     For the moment the remaining space of sector 0 is reserved, I don't know what to put there.
     ---
     Sectors 1 to 16: FREEZONES.
     Since 1 freezone takes 8 bytes (4 bytes address + 4 bytes size), 16 sectors of 512B can handle 1024 free zones.
     So yeah, 8KB of freezones metadata. Not that much after all.
     ---
     After that, here is the journal in sector 17
     The journal is meant to be developped later but I still plan it
     A journal data will be like this:
     ID - 1 byte
     Task - 1 byte
     Address - 4 bytes
     State - 1 byte
     other flags - ONE MORE BYTE (Daft Punk reference)

     So, 8 bytes. An entire sector can then store 64 tasks.
     ---
     Sector 18:
     The trash.
     Since you can trash files before deleting them, here will be noted every trashed file address
     Since a 512B sector can contain 128 files addresses, the trash will be marked as full when 128 files or folders are trashed.
     Note: Deleting a folder will result in the deletion of all its children.
     The trash does not optimize anything and to be honest deleting directly the objects would be faster,
     but maybe in the future I'll try to make the trash more useful.

     */


    /*
     Sector schematics

     Sector 0:
     Offset  Size      Object
     ────────────────────────────────
     0       5         Magic "QOSFS"
     5       16        Version
     21      256       Description
     277     8         EOD
     285     1         Flags
     286     226       Reserved
     ────────────────────────────────
     512
     */



    // INIT SECTOR 0
    I8 buf[512] = {0};

    EOD = 19;
    I8 flags = 0;
    flags |= (1 << 0);

    memcpy(buf, QOSFS_VERSION, 10);
    memcpy(buf, QOSFS_DESCRIPTION, 256);

    write_u64_le(buf, EOD);
    write_u8_le(buf, flags);

    res = disk_write(volume, buf, 0, 1);
    if (res != RES_OK) return QOS_DISKERR;

    // Other sectors are already empty

    if (testparam){
        COMRES res;
        strcpy(CURRENT_PATH, "");
        // We'll just set the start at sector 19, and this also will be our first check:
        char readme[32] = "Welcome to QOSFS!\n";
        res = QOSFS_fmake("README.qos", 1);
        if (res != QOS_OK) return QOS_UNKERR;
        res = QOSFS_fwrite("README.qos", readme);
        if (res != QOS_OK) return QOS_UNKERR;
        res = QOSFS_fclose("README.qos");
        if (res != QOS_OK) return QOS_UNKERR;
    }

    return QOS_OK;
}

I64 find_free_zone(I64 size){
    for (int i = 1023; i < 0; --i){
        if (FREEZONES[i].count <= size){
            I64 toret = FREEZONES[i].addr;
            // Marqué la zone retournée comme utilisée d'avance
            FREEZONES[i].addr = 0;
            FREEZONES[i].count = 0;
            return toret;
        }
    }

    return 0;
}

COMRES QOSFS_fmake(const char* name, _Bool openparam){
    if (strlen(name) > 128) return QOS_INVNAME;
    I64 place = find_free_zone(x(1));
    if (!place){
        return QOS_NOSPACE;
    }
    /*
     File header (512B):
     FIL    file header mark (3 bytes)
     name   name of the file (128 max)
     size   size of the file
     flags:
     fragmented?      0/1   1   Is it splited into parts?
     hidden?          0/1   2   Is it marked as hidden?
     trashed?         0/1   3   Is it currently in the trash?
     read-only?       0/1   4   Is it read-only?
     executable?      0/1   5   Is it an executable binairy (why not?)?
     untouchable?     0/1   6   Is it a important file that QOSFS should NEVER touch in any way?
     extended_header? 0/1   7   Does the next sector is of the same file, or the header is longer than 1 sector.
     // ideas of flag 8 here

     number of extents if any
     parent folder address

     dates:     year 16 bits value other 8 bits
     created year
     created month
     created day
     created hour
     created minute
     created second
     last modified year
     last modified month
     last modified day
     last modified hour
     last modified minute
     last modified second
     last used in any way year
     last used in any way month
     last used in any way day
     last used in any way hour
     last used in any way minute
     last used in any way second
     here, 2+5 so 7 bytes
     7x3 = 21 bytes for these dates

     possibly an extent data here
     address+count (64 bits, so 8B)
     16 bytes per extent data, so 240 would be enough to tell about 15 extents...?

     and after that to byte 506: reserved
     507-511: EOFH
     free space in the header: 91 bytes to put whatever I want + 1 free flag for the future
     */
    I64 filest = place;
    I8 buf[512];
}

COMRES QOSFS_fwrite(const char* path, const char* text){
    //stub
}

COMRES QOSFS_fclose(const char* path){
    //stub
}

void GetErrorInfo(I8* buf){
    buf = "stub bonjour ouais tkt";
}
