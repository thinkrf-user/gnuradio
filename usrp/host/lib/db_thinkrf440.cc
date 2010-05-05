//
// Copyright 2010 ThinkRF, Inc.
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
// Implementation for ThinkRF 400-4000 daughterboard.
//
// Mainly taken from db_wbxng.cc.
//
// Written by: catalin.patulea@thinkrf.com
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <usrp/db_thinkrf440.h>
#include "db_wbxng_adf4350.h"
#include <db_base_impl.h>
#include <stdio.h>

// d'board i/o pin defs
#define RFDC1_VSWA      (1 << 0)
#define RFDC1_VSWB      (1 << 1)
#define RFDC1_VSWC      (1 << 2)
#define RFDC1_VSWD      (1 << 3)  // LED1
#define RFDC1_FILTER_A0 (1 << 4)
#define RFDC1_FILTER_A1 (1 << 5)
#define RDFC1_MASK      ((1 << 6) - 1)

thinkrf440_base::thinkrf440_base(usrp_basic_sptr _usrp, int which, int _power_on)
  : db_base(_usrp, which), d_power_on(_power_on), d_common(NULL)
{
  /*
    @param usrp: instance of usrp.source_c
    @param which: which side: 0 or 1 corresponding to side A or B respectively
    @type which: int
  */

  fprintf(stderr, "thinkrf440_base::thinkrf440_base(which=%d)\n", which);

  usrp()->_write_oe(d_which, RDFC1_MASK, RDFC1_MASK);

  usrp()->write_io(d_which,
    RFDC1_VSWA | // +22 dB
    RFDC1_VSWB | // +22 dB
    RFDC1_VSWC | // +22 dB
    RFDC1_VSWD | // LED
    RFDC1_FILTER_A0 | RFDC1_FILTER_A1, // 45.1 MHz
    0xffff);

  d_first = true;
  d_spi_format = SPI_FMT_MSB | SPI_FMT_HDR_0;
}

thinkrf440_base::~thinkrf440_base()
{
  if (d_common)
    delete d_common;
}

struct freq_result_t
thinkrf440_base::set_freq(double freq)
{
  /*
    @returns (ok, actual_baseband_freq) where:
    ok is True or False and indicates success or failure,
    actual_baseband_freq is the RF frequency that corresponds to DC in the IF.
  */

  freq_t int_freq = freq_t(freq);
  bool ok = d_common->_set_freq(int_freq);
  double freq_result = (double) d_common->_get_freq();
  struct freq_result_t args = {ok, freq_result};

  /* Wait before reading Lock Detect*/
  timespec t;
  t.tv_sec = 0;
  t.tv_nsec = 10000000;
  nanosleep(&t, NULL);

  fprintf(stderr,
    "thinkrf440_base::set_freq(freq=%.03fM) // result: %.03fM (ok=%d)\n",
    freq/1e6, args.baseband_freq/1e6, args.ok
  );
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
thinkrf440_base::_set_pga(float pga_gain)
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
thinkrf440_base::is_quadrature()
{
  /*
    Return True if this board requires both I & Q analog channels.

    This bit of info is useful when setting up the USRP Rx mux register.
  */
  return true;
}

double
thinkrf440_base::freq_min()
{
  return (double) d_common->_get_min_freq()/2.0;
}

double
thinkrf440_base::freq_max()
{
  return (double) d_common->_get_max_freq()/2.0;
}

/**************************************************************************/


thinkrf440_base_rx::thinkrf440_base_rx(usrp_basic_sptr _usrp, int which, int _power_on)
  : thinkrf440_base(_usrp, which, _power_on)
{
  /*
    @param usrp: instance of usrp.source_c
    @param which: 0 or 1 corresponding to side RX_A or RX_B respectively.
  */

  d_common = new adf4350(_usrp, d_which, SPI_ENABLE_VCO);
  
  // Disable VCO/PLL
  d_common->_enable(true);

  // TODO: Initialize daughterboard I/Os
  //usrp()->_write_oe(d_which, ?, ?);
  //usrp()->write_io(d_which,  ?, ?);

  select_rx_antenna("J1");

  bypass_adc_buffers(true);

  /*
  set_lo_offset(-4e6);
  */
}

thinkrf440_base_rx::~thinkrf440_base_rx()
{
  shutdown();
}

void
thinkrf440_base_rx::shutdown()
{
  fprintf(stderr, "thinkrf440_base_rx::shutdown  d_is_shutdown = %d\n", d_is_shutdown);

  if (!d_is_shutdown){
    d_is_shutdown = true;
    // do whatever there is to do to shutdown

    // Power down VCO/PLL
    d_common->_enable(false);

    // fprintf(stderr, "thinkrf440_base_rx::shutdown  before _write_control\n");
    //_write_control(_compute_control_reg());

    usrp()->write_io(d_which, 0, RFDC1_VSWD);
  }
}

bool
thinkrf440_base_rx::set_auto_tr(bool on)
{
  return false;
}

bool
thinkrf440_base_rx::select_rx_antenna(int which_antenna)
{
  return which_antenna == 0;
}

bool
thinkrf440_base_rx::select_rx_antenna(const std::string &which_antenna)
{
  if (which_antenna == "J1") {
    return select_rx_antenna(0);
  } else {
    return false;
  }
}

bool
thinkrf440_base_rx::set_gain(float gain)
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
thinkrf440_base_rx::_set_attn(float attn)
{
  //fprintf(stderr, "Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x \n", attn, attn_code, iobits & ATTN_MASK, ATTN_MASK);
  //return usrp()->write_io(d_which, ?, ?);
  return true;
}

// ----------------------------------------------------------------

db_thinkrf440_rx::db_thinkrf440_rx(usrp_basic_sptr usrp, int which)
  : thinkrf440_base_rx(usrp, which)
{
  set_gain((gain_min() + gain_max()) / 2.0);  // initialize gain
}

db_thinkrf440_rx::~db_thinkrf440_rx()
{
}

float
db_thinkrf440_rx::gain_min()
{
  return usrp()->pga_min();
}

float
db_thinkrf440_rx::gain_max()
{
  return usrp()->pga_max()+30.5;
}

float
db_thinkrf440_rx::gain_db_per_step()
{
  return 0.05;
}


bool
db_thinkrf440_rx::i_and_q_swapped()
{
  return false;
}
