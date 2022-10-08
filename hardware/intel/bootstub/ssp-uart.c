/*
 * ssp-uart.c SPI via SSP
 * Copyright (C) 2011, Mark F. Brown <mark.f.brown@intel.com> Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "ssp-uart.h"
#include "bootstub.h"

#define SSP_TIMEOUT	0xAFF
#define SSP_SLAVE	0x02 /* Slave select */
#define SSP_SSCR0	0x00C0008F
#define SSP_SSCR1	0x10000000

static int ssp_inited = 0;
static volatile struct ssp_reg *pspi = 0;

#define WRITE_DATA                  (2<<14)

static void ssp_init()
{
	pspi = (struct ssp_reg*)TNG_SSP5_ADDR_BASE;
	pspi->SSPx_SSFS = SSP_SLAVE;
	pspi->SSPx_SSCR1 = SSP_SSCR1;
	pspi->SSPx_SSCR0 = SSP_SSCR0;

	ssp_inited = 1;
}

static void ssp_max3110_putc(char c)
{
	vu32 SSCR0 = 0;
	vu32 i;

	pspi = (struct ssp_reg*)TNG_SSP5_ADDR_BASE;
	SSCR0 = (WRITE_DATA | c);
	pspi->SSPx_SSDR = SSCR0;

	for (i = 0; i < SSP_TIMEOUT; i++)
	{
		SSCR0 = pspi->SSPx_SSSR;
		if ((SSCR0 & 0xF00) == 0) break;
	}

	SSCR0 = pspi->SSPx_SSDR;
}

void bs_ssp_printk(const char *str)
{
	if (!str)
		return;

	if (!ssp_inited)
	{
		ssp_init();
	}

	while (*str) {
		if (*str == '\n')
			ssp_max3110_putc('\r');

		ssp_max3110_putc(*str++);
	}
}
