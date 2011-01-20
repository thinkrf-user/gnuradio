#!/usr/bin/env python
#
# Copyright 2004,2005,2006,2007 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

# print "Loading revised usrp_oscope with additional options for scopesink..."

from gnuradio import gr, gru
from gnuradio import usrp
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from gnuradio.wxgui import stdgui2, scopesink2, form, slider
from optparse import OptionParser
import wx
import sys
from usrpm import usrp_dbid


def pick_subdevice(u):
    """
    The user didn't specify a subdevice on the command line.
    If there's a daughterboard on A, select A.
    If there's a daughterboard on B, select B.
    Otherwise, select A.
    """
    if u.db(0, 0).dbid() >= 0:       # dbid is < 0 if there's no d'board or a problem
        return (0, 0)
    if u.db(1, 0).dbid() >= 0:
        return (1, 0)
    return (0, 0)


class app_top_block(stdgui2.std_top_block):
    def __init__(self, frame, panel, vbox, argv):
        stdgui2.std_top_block.__init__(self, frame, panel, vbox, argv)

        self.frame = frame
        self.panel = panel
        
        parser = OptionParser(option_class=eng_option)
        parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=None,
                          help="select USRP Rx side A or B (default=first one with a daughterboard)")
        parser.add_option("-d", "--decim", type="int", default=16,
                          help="set fgpa decimation rate to DECIM [default=%default]")
        parser.add_option("-f", "--freq", type="eng_float", default=None,
                          help="set frequency to FREQ", metavar="FREQ")
        parser.add_option("-g", "--gain", type="string", default="Ultralow",
                          help="set gain value (High, Med, Low, Ultralow), (default is Ultralow)")
        parser.add_option("-8", "--width-8", action="store_true", default=False,
                          help="Enable 8-bit samples across USB")
        parser.add_option( "--no-hb", action="store_true", default=False,
                          help="don't use halfband filter in usrp")
        parser.add_option("-C", "--basic-complex", action="store_true", default=False,
                          help="Use both inputs of a basicRX or LFRX as a single Complex input channel")
        parser.add_option("-D", "--basic-dualchan", action="store_true", default=False,
                          help="Use both inputs of a basicRX or LFRX as seperate Real input channels")
        parser.add_option("-n", "--frame-decim", type="int", default=1,
                          help="set oscope frame decimation factor to n [default=1]")
        parser.add_option("-v", "--v-scale", type="eng_float", default=1000,
                          help="set oscope initial V/div to SCALE [default=%default]")
        parser.add_option("-t", "--t-scale", type="eng_float", default=49e-6,
                          help="set oscope initial s/div to SCALE [default=50us]")
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        self.show_debug_info = True
        
        # build the graph
        if options.basic_dualchan:
          self.num_inputs=2
        else:
          self.num_inputs=1
        if options.no_hb or (options.decim<8):
          #Min decimation of this firmware is 4. 
          #contains 4 Rx paths without halfbands and 0 tx paths.
          self.fpga_filename="std_4rx_0tx.rbf"
          self.u = usrp.source_c(nchan=self.num_inputs,decim_rate=options.decim, fpga_filename=self.fpga_filename)
        else:
          #Min decimation of standard firmware is 8. 
          #standard fpga firmware "std_2rxhb_2tx.rbf" 
          #contains 2 Rx paths with halfband filters and 2 tx paths (the default)
          self.u = usrp.source_c(nchan=self.num_inputs,decim_rate=options.decim)

        if options.rx_subdev_spec is None:
            options.rx_subdev_spec = pick_subdevice(self.u)

        if options.width_8:
            width = 8
            shift = 8
            format = self.u.make_format(width, shift)
            #print "format =", hex(format)
            r = self.u.set_format(format)
            #print "set_format =", r
            
        # determine the daughterboard subdevice we're using
        self.subdev = usrp.selected_subdev(self.u, options.rx_subdev_spec)
        if (options.basic_complex  or options.basic_dualchan ):
          if ((self.subdev.dbid()==usrp_dbid.BASIC_RX) or (self.subdev.dbid()==usrp_dbid.LF_RX)):
            side = options.rx_subdev_spec[0]  # side A = 0, side B = 1
            if options.basic_complex:
              #force Basic_RX and LF_RX in complex mode (use both I and Q channel)
              print "Receiver daughterboard forced in complex mode. Both inputs will combined to form a single complex channel."
              self.dualchan=False
              if side==0:
                self.u.set_mux(0x00000010) #enable adc 0 and 1 to form a single complex input on side A
              else: #side ==1
                self.u.set_mux(0x00000032) #enable adc 3 and 2 to form a single complex input on side B
            elif options.basic_dualchan:
              #force Basic_RX and LF_RX in dualchan mode (use input A  for channel 0 and input B for channel 1)
              print "Receiver daughterboard forced in dualchannel mode. Each input will be used to form a seperate channel."
              self.dualchan=True
              if side==0:
                self.u.set_mux(gru.hexint(0xf0f0f1f0)) #enable adc 0, side A to form a real input on channel 0 and adc1,side A to form a real input on channel 1
              else: #side ==1
                self.u.set_mux(0xf0f0f3f2) #enable adc 2, side B to form a real input on channel 0 and adc3,side B to form a real input on channel 1 
          else:
            sys.stderr.write('options basic_dualchan or basic_complex is only supported for Basic Rx or LFRX at the moment\n')
            sys.exit(1)
        else:
          self.dualchan=False
          self.u.set_mux(usrp.determine_rx_mux_value(self.u, options.rx_subdev_spec))

        input_rate = self.u.adc_freq() / self.u.decim_rate()

        self.scope = scopesink2.scope_sink_c(panel, sample_rate=input_rate,
                                            frame_decim=options.frame_decim,
                                            v_scale=options.v_scale,
                                            t_scale=options.t_scale,
                                            num_inputs=self.num_inputs)
        self.connect(self.u, self.scope)

        self._build_gui(vbox)

        # set initial values

        if options.gain is None:
            # if no gain was specified, use Ultralow
            options.gain = "Ultralow"

        if options.freq is None:
            if ((self.subdev.dbid()==usrp_dbid.BASIC_RX) or (self.subdev.dbid()==usrp_dbid.LF_RX)):
              #for Basic RX and LFRX if no freq is specified you probably want 0.0 Hz and not 45 GHz
              options.freq=0.0
            else:
              # if no freq was specified, use the mid-point
              r = self.subdev.freq_range()
              options.freq = float(r[0]+r[1])/2

        self.set_gain(options.gain)

        if self.show_debug_info:
            self.myform['decim'].set_value(self.u.decim_rate())
            self.myform['fs@usb'].set_value(self.u.adc_freq() / self.u.decim_rate())
                        
        if not(self.set_freq(options.freq)):
            self._set_status_msg("Failed to set initial frequency")


    def _set_status_msg(self, msg):
        self.frame.GetStatusBar().SetStatusText(msg, 0)

    def _build_gui(self, vbox):

        def _form_set_freq(kv):
            return self.set_freq(kv['freq'])

        def _form_set_decim(kv):
            return self.set_decim(kv['decim'])

        vbox.Add(self.scope.win, 10, wx.EXPAND)
        
        # add control area at the bottom
        self.myform = myform = form.form()
        hbox = wx.BoxSizer(wx.HORIZONTAL)

        # add our settings box
        vbox1 = wx.BoxSizer(wx.HORIZONTAL)
        myform['freq'] = form.float_field(
            parent=self.panel, sizer=vbox1, label="Center freq", weight=1,
            callback=myform.check_input_and_call(_form_set_freq, self._set_status_msg))

        myform['decim'] = form.int_field(
            parent=self.panel, sizer=vbox1, label="Decim",
            callback=myform.check_input_and_call(_form_set_decim, self._set_status_msg))

        myform['fs@usb'] = form.static_float_field(
            parent=self.panel, sizer=vbox1, label="Fs@USB")

        hbox.Add(vbox1, 1, wx.HORIZONTAL)

        hbox.Add((25,0), 0, 0)

        # add our gain box
        vbox2 = wx.BoxSizer(wx.VERTICAL)
        myform['gain'] = form.radiobox_field(
          parent=self.panel, 
          sizer=vbox2, 
          label="Gain",
          weight=1,
          choices=["High", "Med", "Low", "Ultralow"],
          value="Ultralow",
          callback=self.set_gain
        )
        hbox.Add(vbox2, 0, wx.EXPAND)
        hbox.Add((25,0), 0, 0)

        # now add that big row to the main elements
        vbox.Add(hbox, 0, wx.EXPAND)


        
    def set_freq(self, target_freq):
        """
        Set the center frequency we're interested in.

        @param target_freq: frequency in Hz
        @rypte: bool

        Tuning is a two step process.  First we ask the front-end to
        tune as close to the desired frequency as it can.  Then we use
        the result of that operation and our target_frequency to
        determine the value for the digital down converter.
        """
        r = usrp.tune(self.u, 0, self.subdev, target_freq)
        
        if r:
            self.myform['freq'].set_value(target_freq)     # update displayed value
            return True

        return False

    def set_gain(self, gain):
        self.myform['gain'].set_value(gain)     # update displayed value
        if (gain == "High"):
          self.subdev.set_gain(1)

        if (gain == "Med"):
          self.subdev.set_gain(2)

        if (gain == "Low"):
          self.subdev.set_gain(3)

        if (gain == "Ultralow"):
          self.subdev.set_gain(4)

    def set_decim(self, decim):
        ok = self.u.set_decim_rate(decim)
        if not ok:
            print "set_decim failed"
        input_rate = self.u.adc_freq() / self.u.decim_rate()
        self.scope.set_sample_rate(input_rate)
        if self.show_debug_info:  # update displayed values
            self.myform['decim'].set_value(self.u.decim_rate())
            self.myform['fs@usb'].set_value(self.u.adc_freq() / self.u.decim_rate())
        return ok

def main ():
    app = stdgui2.stdapp(app_top_block, "USRP O'scope", nstatus=1)
    app.MainLoop()

if __name__ == '__main__':
    main ()
