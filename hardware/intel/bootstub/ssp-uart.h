#ifndef _SSP_UART
#define _SSP_UART

#include "types.h"

#define TNG_SSP5_ADDR_BASE      0xFF189000

struct ssp_reg {
	vu32 SSPx_SSCR0;      // 0x00
	vu32 SSPx_SSCR1;      // 0x04
	vu32 SSPx_SSSR;       // 0x08
	vu32 SSPx_SSITR;      // 0x0C
	vu32 SSPx_SSDR;       // 0x10
	vu32 SSPx_DUMMY1;     //0x14
	vu32 SSPx_DUMMY2;     //0x18
	vu32 SSPx_DUMMY3;     //0x1c
	vu32 SSPx_DUMMY4;     //0x20
	vu32 SSPx_DUMMY5;     //0x24
	vu32 SSPx_SSTO;       // 0x28
	vu32 SSPx_SSPSP;      // 0x2C
	vu32 SSPx_SSTSA;      // 0x30
	vu32 SSPx_SSRSA;      // 0x34
	vu32 SSPx_SSTSS;      // 0x38
	vu32 SSPx_SSACD;      // 0x3C
	vu32 SSPx_SSCR2;      // 0x40
	vu32 SSPx_SSFS;       // 0x44
	vu32 SSPx_FRAME_CNT;  // 0x48
};

extern void bs_ssp_printk(const char *str);

#endif
