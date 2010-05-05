/* -*- c++ -*- */
//
// Copyright 2010 ThinkRF, Inc.
// Copyright 2009 Free Software Foundation, Inc.
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
// Include file for ThinkRF 400-4000 daughterboard.
//
// Mostly taken from db_wbxng.h
//
// Written by: catalin.patulea@thinkrf.com
//

#ifndef INCLUDED_DB_THINKRF440_H
#define INCLUDED_DB_THINKRF440_H

#include <usrp/db_base.h>
#include <cmath>

class adf4350;

class thinkrf440_base : public db_base
{
public:
  thinkrf440_base(usrp_basic_sptr usrp, int which, int _power_on=0);
  ~thinkrf440_base();

  struct freq_result_t set_freq(double freq);

  bool  is_quadrature();
  double freq_min();
  double freq_max();

protected:
  bool _lock_detect();
  bool _set_pga(float pga_gain);

  int power_on() { return d_power_on; }
  int power_off() { return 0; }

  bool d_first;
  int  d_spi_format;
  int  d_spi_enable;
  int  d_power_on;
  int  d_PD;

  adf4350 *d_common;
};

// ----------------------------------------------------------------

class thinkrf440_base_rx : public thinkrf440_base
{
protected:
  void shutdown();
  bool _set_attn(float attn);

public:
  thinkrf440_base_rx(usrp_basic_sptr usrp, int which, int _power_on=0);
  ~thinkrf440_base_rx();

  bool set_auto_tr(bool on);
  bool select_rx_antenna(int which_antenna);
  bool select_rx_antenna(const std::string &which_antenna);
  bool set_gain(float gain);
};

// ----------------------------------------------------------------

class db_thinkrf440_rx : public thinkrf440_base_rx
{
public:
  db_thinkrf440_rx(usrp_basic_sptr usrp, int which);
  ~db_thinkrf440_rx();

  float gain_min();
  float gain_max();
  float gain_db_per_step();
  bool i_and_q_swapped();
};

#endif /* INCLUDED_DB_THINKRF440_H */
