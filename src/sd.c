#include "sd.h"

/* External memset from kernel_main */
extern void memset(void *s, int c, int n);

/* Stub SD card driver - reads sectors from a virtual disk in QEMU */
int sd_readblock(uint32_t sector, char *buffer, uint32_t count) {
    /* This is a placeholder. In a real implementation, this would:
     * 1. Use ATA/SATA commands to read from the disk
     * 2. Or use SD card protocol if on real hardware
     * 
     * For QEMU testing with -hda rootfs.img, we would use
     * disk I/O routines to access the disk image.
     * 
     * For now, return success but with zeroed buffer
     */
    memset(buffer, 0, count * 512);
    return 0;
}
