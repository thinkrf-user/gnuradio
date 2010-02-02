/*
 * USRP - Universal Software Radio Peripheral
 *
 * Copyright (C) 2003 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Boston, MA  02110-1301  USA
 */

/*
 * These are the register definitions for the ThinkRF WSA1000 Rev 2 board.
 */

#ifndef _USRP_REV1_REGS_H_
#define _USRP_REV1_REGS_H_

#include "fx2regs.h"

/*
 * Port A (bit addressable):
 */

#define USRP_PA			IOA		// Port A
#define	USRP_PA_OE		OEA		// Port A direction register

/* TODO: What are J4[15] and J4[19] */
#define	bmPA_INT_PCA		bmBIT0		// I2C GPIO (U30) interrupt
#define	bmPA_J4_15		bmBIT1		// daughterboard I/O
#define	bmPA_SEN_FPGA		bmBIT2		// serial enable for FPGA (active low)
#define	bmPA_FPGA_RDY		bmBIT3		// ready status from FPGA (active high)
#define	bmPA_S_CLK		bmBIT4		// SPI serial clock
#define	bmPA_S_DATA_FROM_PERIPH	bmBIT5		// SPI SDO (peripheral rel name)
#define	bmPA_S_DATA_TO_PERIPH	bmBIT6		// SPI SDI (peripheral rel name)
#define	bmPA_J4_19		bmBIT7		// daughterboard I/O


sbit at 0x80+4 bitS_CLK;		// 0x80 is the bit address of PORT A
sbit at 0x80+5 bitS_IN;			// in from FX2 point of view
sbit at 0x80+6 bitS_OUT;		// out from FX2 point of view


#define	bmPORT_A_OUTPUTS  (bmPA_SEN_FPGA		\
			   | bmPA_S_CLK			\
			   | bmPA_S_DATA_TO_PERIPH	\
			   )

#define	bmPORT_A_INITIAL   (bmPA_SEN_FPGA)


/*
 * Port B: GPIF	FD[7:0]
 */

/*
 * Port C: J4[6:2:20] daughterboard I/Os (unused)
 */
#define bmPC_EEPROM_BOOT	bmBIT0		// flashes after eeprom boot
#define bmPC_TICK		bmBIT1		// 2 Hz from timer2 interrupt

#define bmPORT_C_OUTPUTS  0xFF
#define bmPORT_C_INITIAL  0x00

/*
 * Port D: GPIF	FD[15:8]
 */

/*
 * Port E: J4[35:2:49] daughterboard I/Os (unused)
 */
#define bmPORT_E_OUTPUTS  0xFF
#define bmPORT_E_INITIAL  0x00

/*
 * FPGA output lines that are tied to FX2 RDYx inputs.
 * These are readable using GPIFREADYSTAT.
 */
#define	bmFPGA_HAS_SPACE		bmBIT0	// EF#
#define	bmFPGA_PKT_AVAIL		bmBIT1	// FF#

/*
 * FPGA input lines that are tied to the FX2 CTLx outputs.
 *
 * These are controlled by the GPIF microprogram...
 */
#define bmFPGA_REN			bmBIT0
#define bmFPGA_OE			bmBIT1
#define bmFPGA_U8			bmBIT2	// unused

#endif /* _USRP_REV1_REGS_H_ */
