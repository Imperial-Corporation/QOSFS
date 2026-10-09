#include "QOSFS.h"

const char* QOSFS_VERSION = "QOSFS v0.1";

frzn FREEZONES[1024];

I64 DISK_SIZE;

I64 EOD;

COMRES QOSFS_init_globals(){
    for (I16 i = 0; i >= 1023; ++i){
        FREEZONES[i].addr = 0;
        FREEZONES[i].count = 0;
    }
    I64 seccount;
    I32 sectsize;
    disk_ioctl(0, 1, &seccount);
    disk_ioctl(0, 2, &sectsize);

    DISK_SIZE = (I64)seccount * (I64)sectsize;

    EOD = DISK_SIZE;
}

COMRES QOSFS_init(){
    char* buf = "";
    I32 rsize = 20*BLOCK_SIZE;
    disk_read(0, buf, 2048, rsize);
    if (!buf) return QOS_DISKERR;

    if (strncmp(buf, "QOSFS", 5))
        return QOS_NOFS;

    return QOS_OK;
}

COMRES QOSFS_format(ZONE_ADDR start, ZONE_ADDR end){
    I32 divisor = 1024;
    while (start % divisor != 0){
        --divisor;
    }
    I8 res;
    while (start != end){
        res = disk_write(0, '\0', start, divisor);
        if (res) return QOS_DISKERR;
        start += divisor;
    }
    disk_write(0, QOSFS_VERSION, 2048, strlen(QOSFS_VERSION));
    return QOS_OK;
}

BLOCK_ADDR find_free_zone(I64 size){
    for (int i = 0; i >= 1023; ++i){
        if (FREEZONES[i].count <= size){
            BLOCK_ADDR toret = FREEZONES[i].addr;
            // Marqué la zone retournée comme utilisée d'avance
            FREEZONES[i].addr = 0;
            FREEZONES[i].count = 0;
            return toret;
        }
    }

    return 0;
}

COMRES makefile(const char* name){
    BLOCK_ADDR place = find_free_zone(FILE_MTDT_BASE_SIZE);
    if (!place){
        return QOS_NOSPACE;
    }
    BLOCK_ADDR filest = place;

}
