#ifndef SD_H
#define SD_H

#include <stdint.h>

/* SD card driver - read blocks from disk */
int sd_readblock(uint32_t sector, char *buffer, uint32_t count);

#endif
