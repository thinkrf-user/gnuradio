/* 
 * USRP - Universal Software Radio Peripheral
 *
 * Copyright (C) 2003,2004 Free Software Foundation, Inc.
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

#include "usrp_common.h"
#include "usrp_commands.h"
#include "fpga.h"
#include "usrp_gpif_inline.h"
#include "timer.h"
#include "i2c.h"
#include "isr.h"
#include "usb_common.h"
#include "fx2utils.h"
#include "usrp_globals.h"
#include "usrp_i2c_addr.h"
#include <string.h>
#include "spi.h"
#include "eeprom_io.h"
#include "usb_descriptors.h"

/*
 * offsets into boot eeprom for configuration values
 */
#define	HW_REV_OFFSET		  5
#define SERIAL_NO_OFFSET	248
#define SERIAL_NO_LEN		  8


#define	bRequestType	SETUPDAT[0]
#define	bRequest	SETUPDAT[1]
#define	wValueL		SETUPDAT[2]
#define	wValueH		SETUPDAT[3]
#define	wIndexL		SETUPDAT[4]
#define	wIndexH		SETUPDAT[5]
#define	wLengthL	SETUPDAT[6]
#define	wLengthH	SETUPDAT[7]


unsigned char g_rx_enable = 0;
unsigned char g_rx_overrun = 0;

/*
 * the host side fpga loader code pushes an MD5 hash of the bitstream
 * into hash1.
 */
#define	  USRP_HASH_SIZE      16
xdata at USRP_HASH_SLOT_1_ADDR unsigned char hash1[USRP_HASH_SIZE];

static void
get_ep0_data (void)
{
  EP0BCL = 0;			// arm EP0 for OUT xfer.  This sets the busy bit

  while (EP0CS & bmEPBUSY)	// wait for busy to clear
    ;
}

/*
 * Handle our "Vendor Extension" commands on endpoint 0.
 * If we handle this one, return non-zero.
 */
unsigned char
app_vendor_cmd (void)
{
  if (bRequestType == VRT_VENDOR_IN){

    /////////////////////////////////
    //    handle the IN requests
    /////////////////////////////////

    switch (bRequest){

    case VRQ_GET_STATUS:
      switch (wIndexL){

      case GS_RX_OVERRUN:
	EP0BUF[0] = g_rx_overrun;
	g_rx_overrun = 0;
	EP0BCH = 0;
	EP0BCL = 1;
	break;

      default:
	return 0;
      }
      break;

    case VRQ_I2C_READ:
      if (!i2c_read (wValueL, EP0BUF, wLengthL))
	return 0;

      EP0BCH = 0;
      EP0BCL = wLengthL;
      break;
      
    case VRQ_SPI_READ:
      if (!spi_read (wValueH, wValueL, wIndexH, wIndexL, EP0BUF, wLengthL))
	return 0;

      EP0BCH = 0;
      EP0BCL = wLengthL;
      break;

    default:
      return 0;
    }
  }

  else if (bRequestType == VRT_VENDOR_OUT){

    /////////////////////////////////
    //    handle the OUT requests
    /////////////////////////////////

    switch (bRequest){

    case VRQ_FPGA_SET_RX_ENABLE:
      fpga_set_rx_enable (wValueL);
      break;

    case VRQ_FPGA_SET_RX_RESET:
      fpga_set_rx_reset (wValueL);
      break;

    case VRQ_I2C_WRITE:
      get_ep0_data ();
      if (!i2c_write (wValueL, EP0BUF, EP0BCL))
	return 0;
      break;

    case VRQ_SPI_WRITE:
      get_ep0_data ();
      if (!spi_write (wValueH, wValueL, wIndexH, wIndexL, EP0BUF, EP0BCL))
	return 0;
      break;

    default:
      return 0;
    }

  }
  else
    return 0;    // invalid bRequestType

  return 1;
}



static void
main_loop (void)
{
  setup_flowstate_common ();

  while (1){

    if (usb_setup_packet_avail ())
      usb_handle_setup_packet ();
    
  
    /*
    if (GPIFTRIG & bmGPIF_IDLE){

      // OK, GPIF is idle.  Let's try to give it some work.

      // First check for overruns

      if (UC_BOARD_HAS_FPGA && (GPIFREADYSTAT & bmFPGA_RX_OVERRUN)){
      
	// record the over run
	if (GPIFREADYSTAT & bmFPGA_RX_OVERRUN)
	  g_rx_overrun = 1;

	// tell the FPGA to clear the flags
	fpga_clear_flags ();
      }

      // See if there are any requests for "IN" packets, and if so
      // whether the FPGA's got any packets for us.

      if (g_rx_enable && !(EP6CS & bmEPFULL)){	// USB end point fifo is not full...

	if (fpga_has_packet_avail ()){		// ... and FPGA has packet available

	  GPIFTCB1 = 0x01;	SYNCDELAY;
	  GPIFTCB0 = 0x00;	SYNCDELAY;

	  setup_flowstate_read ();

	  SYNCDELAY;
	  GPIFTRIG = bmGPIF_EP6_START | bmGPIF_READ; 	// start the xfer
	  SYNCDELAY;

	  while (!(GPIFTRIG & bmGPIF_IDLE)){
	    // wait for the transaction to complete
	  }

	  SYNCDELAY;
	  INPKTEND = 6;	// tell USB we filled buffer (6 is our endpoint num)
	}
      }
    }
    */
  }
}


void spi_test(const unsigned char xdata buffer[8]);

/*
 * called at 100 Hz from timer2 interrupt
 *
 * Toggle led 0
 */
void
isr_tick (void) interrupt
{
  static unsigned char	count = 1;
  static unsigned char xdata buffer[8];
  
  if (--count == 0){
    count = 50;
    IOC ^= bmPC_TICK;
  }
  
  buffer[0] = 0x7f;
  buffer[4] = 0xff;
  buffer[7] = count;
  spi_test(buffer);

  clear_timer_irq ();
}

/*
 * Read h/w rev code and serial number out of boot eeprom and
 * patch the usb descriptors with the values.
 */
void
patch_usb_descriptors(void)
{
  usb_desc_hw_rev_binary_patch_location_0[0] = 5;
  usb_desc_hw_rev_binary_patch_location_1[0] = 5;
  usb_desc_hw_rev_ascii_patch_location_0[0] = '5';
}

void
main (void)
{
#if 0
  g_rx_enable = 0;	// FIXME (work around initialization bug)
  g_rx_overrun = 0;
#endif

  memset (hash1, 0, USRP_HASH_SIZE);	// zero fpga bitstream hash.  This forces reload
  
  init_usrp ();
  init_gpif ();
  
  // if (UC_START_WITH_GSTATE_OUTPUT_ENABLED)
  IFCONFIG |= bmGSTATE;			// no conflict, start with it on

  EA = 0;		// disable all interrupts

  patch_usb_descriptors();

  setup_autovectors ();
  usb_install_handlers ();
  hook_timer_tick ((unsigned short) isr_tick);

  EIEX4 = 1;		// disable INT4 FIXME
  EA = 1;		// global interrupt enable

  fx2_renumerate ();	// simulates disconnect / reconnect

  main_loop ();
}
