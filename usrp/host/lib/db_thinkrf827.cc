//
// Copyright 2008,2009 Free Software Foundation, Inc.
//
// This file is part of GNU Radio
//
// GNU Radio is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either asversion 3, or (at your option)
// any later version.
//
// GNU Radio is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Radio; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street,
// Boston, MA 02110-1301, USA.

//
// Implementation for ThinkRF 800-2700 daughterboard.
//
// Mainly taken from db_wbxng.cc.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <usrp/db_thinkrf827.h>
#include "db_wbxng_adf4350.h"
#include <db_base_impl.h>
#include <stdio.h>

// d'board i/o pin defs
// Tx and Rx have shared defs, but different i/o regs
#define ENABLE_5        (1 << 7)         // enables 5.0V power supply
#define ENABLE_33       (1 << 6)         // enables 3.3V supply
//#define RX_TXN          (1 << 15)         // Tx only: T/R antenna switch for TX/RX port
//#define RX2_RX1N        (1 << 15)         // Rx only: antenna switch between RX2 and TX/RX port
#define RX_TXN          ((1 << 5)|(1 << 15))         // Tx only: T/R antenna switch for TX/RX port
#define RX2_RX1N        ((1 << 5)|(1 << 15))         // Rx only: antenna switch between RX2 and TX/RX port
#define RXBB_EN         (1 << 4)
#define TXMOD_EN        (1 << 4)
#define PLL_CE          (1 << 3)
#define PLL_PDBRF       (1 << 2)
#define PLL_MUXOUT      (1 << 1)
#define PLL_LOCK_DETECT (1 << 0)

// RX Attenuator constants
#define ATTN_SHIFT	8
#define ATTN_MASK	(63 << ATTN_SHIFT)

thinkrf827_base::thinkrf827_base(usrp_basic_sptr _usrp, int which, int _power_on)
  : db_base(_usrp, which), d_power_on(_power_on)
{
  /*
    @param usrp: instance of usrp.source_c
    @param which: which side: 0 or 1 corresponding to side A or B respectively
    @type which: int
  */

  usrp()->_write_oe(d_which, 0, 0xffff);   // turn off all outputs

  d_first = true;
  d_spi_format = SPI_FMT_MSB | SPI_FMT_HDR_0;

  _enable_refclk(false);                // disable refclk

  set_auto_tr(false);
}

thinkrf827_base::~thinkrf827_base()
{
  if (d_common)
    delete d_common;
}

struct freq_result_t
thinkrf827_base::set_freq(double freq)
{
  /*
    @returns (ok, actual_baseband_freq) where:
    ok is True or False and indicates success or failure,
    actual_baseband_freq is the RF frequency that corresponds to DC in the IF.
  */

  freq_t int_freq = freq_t(freq);
  bool ok = d_common->_set_freq(int_freq*2);
  double freq_result = (double) d_common->_get_freq()/2.0;
  struct freq_result_t args = {ok, freq_result};

  /* Wait before reading Lock Detect*/
  timespec t;
  t.tv_sec = 0;
  t.tv_nsec = 10000000;
  nanosleep(&t, NULL);

  //fprintf(stderr,"Setting WBXNG frequency, requested %d, obtained %f, lock_detect %d\n",
  //        int_freq, freq_result, d_common->_get_locked());

  // FIXME
  // Offsetting the LO helps get the Tx carrier leakage out of the way.
  // This also ensures that on Rx, we're not getting hosed by the
  // FPGA's DC removal loop's time constant.  We were seeing a
  // problem when running with discontinuous transmission.
  // Offsetting the LO made the problem go away.
  //freq += d_lo_offset;

  return args;
}

bool
thinkrf827_base::_set_pga(float pga_gain)
{
  if(d_which == 0) {
    usrp()->set_pga(0, pga_gain);
    usrp()->set_pga(1, pga_gain);
  }
  else {
    usrp()->set_pga(2, pga_gain);
    usrp()->set_pga(3, pga_gain);
  }
  return true;
}

bool
thinkrf827_base::is_quadrature()
{
  /*
    Return True if this board requires both I & Q analog channels.

    This bit of info is useful when setting up the USRP Rx mux register.
  */
  return true;
}

double
thinkrf827_base::freq_min()
{
  return (double) d_common->_get_min_freq()/2.0;
}

double
thinkrf827_base::freq_max()
{
  return (double) d_common->_get_max_freq()/2.0;
}

/**************************************************************************/


thinkrf827_base_rx::thinkrf827_base_rx(usrp_basic_sptr _usrp, int which, int _power_on)
  : thinkrf827_base(_usrp, which, _power_on)
{
  /*
    @param usrp: instance of usrp.source_c
    @param which: 0 or 1 corresponding to side RX_A or RX_B respectively.
  */

  if(which == 0) {
    d_spi_enable = SPI_ENABLE_RX_A;
  }
  else {
    d_spi_enable = SPI_ENABLE_RX_B;
  }

  d_common = new adf4350(_usrp, d_which, d_spi_enable);
  
  // Disable VCO/PLL
  d_common->_enable(true);

  usrp()->_write_oe(d_which, (RX2_RX1N|RXBB_EN|ATTN_MASK|ENABLE_33|ENABLE_5), (RX2_RX1N|RXBB_EN|ATTN_MASK|ENABLE_33|ENABLE_5));
  usrp()->write_io(d_which,  (power_on()|RX2_RX1N|RXBB_EN|ENABLE_33|ENABLE_5), (RX2_RX1N|RXBB_EN|ATTN_MASK|ENABLE_33|ENABLE_5));
  //fprintf(stderr,"Setting WBXNG RXBB on");

  // set up for RX on TX/RX port
  select_rx_antenna("TX/RX");

  bypass_adc_buffers(true);

  /*
  set_lo_offset(-4e6);
  */
}

thinkrf827_base_rx::~thinkrf827_base_rx()
{
  shutdown();
}

void
thinkrf827_base_rx::shutdown()
{
  // fprintf(stderr, "thinkrf827_base_rx::shutdown  d_is_shutdown = %d\n", d_is_shutdown);

  if (!d_is_shutdown){
    d_is_shutdown = true;
    // do whatever there is to do to shutdown

    // Power down VCO/PLL
    d_common->_enable(false);

    // fprintf(stderr, "thinkrf827_base_rx::shutdown  before _write_control\n");
    //_write_control(_compute_control_reg());

    // fprintf(stderr, "thinkrf827_base_rx::shutdown  before _enable_refclk\n");
    _enable_refclk(false);                       // turn off refclk

    // fprintf(stderr, "thinkrf827_base_rx::shutdown  before set_auto_tr\n");
    set_auto_tr(false);

    // Power down
    usrp()->write_io(d_which, power_off(), (RX2_RX1N|RXBB_EN|ATTN_MASK|ENABLE_33|ENABLE_5));

    // fprintf(stderr, "thinkrf827_base_rx::shutdown  after set_auto_tr\n");
  }
}

bool
thinkrf827_base_rx::set_auto_tr(bool on)
{
  bool ok = true;
  if(on) {
    ok &= set_atr_mask (RXBB_EN|RX2_RX1N);
    ok &= set_atr_txval(      0|RX2_RX1N);
    ok &= set_atr_rxval(RXBB_EN|       0);
  }
  else {
    ok &= set_atr_mask (0);
    ok &= set_atr_txval(0);
    ok &= set_atr_rxval(0);
  }
  return true;
}

bool
thinkrf827_base_rx::select_rx_antenna(int which_antenna)
{
  /*
    Specify which antenna port to use for reception.
    @param which_antenna: either 'TX/RX' or 'RX2'
  */

  if(which_antenna == 0) {
    usrp()->write_io(d_which, 0,RX2_RX1N);
  }
  else if(which_antenna == 1) {
    usrp()->write_io(d_which, RX2_RX1N, RX2_RX1N);
  }
  else {
    return false;
  }
  return true;
}

bool
thinkrf827_base_rx::select_rx_antenna(const std::string &which_antenna)
{
  /*
    Specify which antenna port to use for reception.
    @param which_antenna: either 'TX/RX' or 'RX2'
  */


  if(which_antenna == "TX/RX") {
    usrp()->write_io(d_which, 0, RX2_RX1N);
  }
  else if(which_antenna == "RX2") {
    usrp()->write_io(d_which, RX2_RX1N, RX2_RX1N);
  }
  else {
    return false;
  }

  return true;
}

bool
thinkrf827_base_rx::set_gain(float gain)
{
  /*
    Set the gain.

    @param gain:  gain in decibels
    @returns True/False
  */

  // clamp gain
  gain = std::max(gain_min(), std::min(gain, gain_max()));

  float pga_gain, agc_gain;

  float maxgain = gain_max() - usrp()->pga_max();
  float mingain = gain_min();
  if(gain > maxgain) {
    pga_gain = gain-maxgain;
    assert(pga_gain <= usrp()->pga_max());
    agc_gain = maxgain;
  }
  else {
    pga_gain = 0;
    agc_gain = gain;
  }

  return _set_attn(maxgain-agc_gain) && _set_pga(int(pga_gain));
}

bool
thinkrf827_base_rx::_set_attn(float attn)
{
  int attn_code = int(floor(attn/0.5));
  unsigned int iobits = (~attn_code) << ATTN_SHIFT;
  //fprintf(stderr, "Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x \n", attn, attn_code, iobits & ATTN_MASK, ATTN_MASK);
  return usrp()->write_io(d_which, iobits, ATTN_MASK);
}

// ----------------------------------------------------------------

db_thinkrf827_rx::db_thinkrf827_rx(usrp_basic_sptr usrp, int which)
  : thinkrf827_base_rx(usrp, which)
{
  set_gain((gain_min() + gain_max()) / 2.0);  // initialize gain
}

db_thinkrf827_rx::~db_thinkrf827_rx()
{
}

float
db_thinkrf827_rx::gain_min()
{
  return usrp()->pga_min();
}

float
db_thinkrf827_rx::gain_max()
{
  return usrp()->pga_max()+30.5;
}

float
db_thinkrf827_rx::gain_db_per_step()
{
  return 0.05;
}


bool
db_thinkrf827_rx::i_and_q_swapped()
{
  return false;
}
