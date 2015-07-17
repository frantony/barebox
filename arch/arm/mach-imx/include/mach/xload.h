#ifndef __MACH_XLOAD_H
#define __MACH_XLOAD_H

int imx6_spi_load_image(int instance, unsigned int flash_offset, void *buf, int len);
int imx6_spi_start_image(int instance);

#endif /* __MACH_XLOAD_H */
