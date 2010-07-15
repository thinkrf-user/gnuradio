/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_AD9640_H
#define INCLUDED_AD9640_H

/*
 * Analog Devices AD9640 registers and some fields
 */

#define BEGIN_AD9640	namespace ad9640 {
#define	END_AD964	}
#define	DEF static const int

BEGIN_AD9640;

DEF REG_CLOCK_DIVIDE = 0x0b;

DEF REG_DEVICE_UPDATE = 0xff;
DEF DEVICE_UPDATE_TRANSFER = 0x01;

END_AD964;

#undef DEF
#undef BEGIN_AD9640
#undef END_AD964

#endif /* INCLUDED_AD9640_H */
