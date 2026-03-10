/* -*- c++ -*- */
/* 
 * Copyright 2015 <Nikos Kargas (nkargas@isc.tuc.gr)>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_RFID_GLOBAL_VARS_H
#define INCLUDED_RFID_GLOBAL_VARS_H

#include <rfid/api.h>
#include <map>
#include <sys/time.h>

namespace gr {
  namespace rfid {

    enum STATUS               {RUNNING, TERMINATED};
    enum GEN2_LOGIC_STATUS  {SEND_QUERY, SEND_ACK, SEND_QUERY_REP, IDLE, SEND_CW, SEND_CW_2,START, SEND_QUERY_ADJUST, SEND_NAK_QR, SEND_NAK_Q, POWER_DOWN}; 
    enum GATE_STATUS        {GATE_OPEN, GATE_CLOSED, GATE_SEEK_RN16, GATE_SEEK_EPC};  
    enum DECODER_STATUS     {DECODER_DECODE_RN16, DECODER_DECODE_EPC};
    
    struct READER_STATS
    {
      int n_queries_sent;

      int cur_inventory_round;
      int cur_slot_number;

      int max_slot_number;
      int max_inventory_round;
      int n_epc_correct;
      int h_flag;
      float h_h = 0.2;
      float h_l = 0.1;
      float scale;
      std::vector <int>  unique_tags_round;
      std::map<int,int> tag_reads;    

      struct timeval start, end; 
    };

    struct READER_STATE
    {
      STATUS               status;
      GEN2_LOGIC_STATUS   gen2_logic_status;
      GATE_STATUS         gate_status;
      DECODER_STATUS       decoder_status;
      READER_STATS         reader_stats;



      std::vector<float> magn_squared_samples; // used for sync
      int n_samples_to_ungate; // used by the GATE and DECODER block
    };



    // CONSTANTS (READER CONFIGURATION)

    // Fixed number of slots (2^(FIXED_Q))  
    const int FIXED_Q              = 0;

    // Termination criteria
    // const int MAX_INVENTORY_ROUND = 50;
    const int MAX_NUM_QUERIES     = 1000;     // Stop after MAX_NUM_QUERIES have been sent

    // valid values for Q
    const int Q_VALUE [16][4] =  
    {
        {0,0,0,0}, {0,0,0,1}, {0,0,1,0}, {0,0,1,1}, 
        {0,1,0,0}, {0,1,0,1}, {0,1,1,0}, {0,1,1,1}, 
        {1,0,0,0}, {1,0,0,1}, {1,0,1,0}, {1,0,1,1},
        {1,1,0,0}, {1,1,0,1}, {1,1,1,0}, {1,1,1,1}
    };  

    const bool P_DOWN = false;

    // Duration in us
    const int CW_D         = 250;    // Carrier wave
    const int P_DOWN_D     = 2000;    // power down
    const int T1_D         = 250;    // Time from Interrogator transmission to Tag response (250 us)
    const int T2_D         = 500;    // Time from Tag response to Interrogator transmission. Max value = 20.0 * T_tag = 500us 
    const int PW_D         = 12;      // Half Tari 
    const int DELIM_D       = 12;      // A preamble shall comprise a fixed-length start delimiter 12.5us +/-5%
    const int TRCAL_D     = 200;    // BLF = DR/TRCAL => 40e3 = 8/TRCAL => TRCAL = 200us
    const int RTCAL_D     = 72;      // 6*PW = 72us

    const int NUM_PULSES_COMMAND = 5;       // Number of pulses to detect a reader command
    const int NUMBER_UNIQUE_TAGS = 100;      // Stop after NUMBER_UNIQUE_TAGS have been read 


    // Number of bits
    const int PILOT_TONE          = 12;  // Optional
    const int TAG_PREAMBLE_BITS  = 6;   // Number of preamble bits
    const int RN16_BITS          = 17;  // Dummy bit at the end
    const int EPC_BITS            = 129;  // PC + EPC + CRC16 + Dummy = 6 + 16 + 96 + 16 + 1 = 135
    const int QUERY_LENGTH        = 22;  // Query length in bits
    
    const int T_READER_FREQ = 40e3;     // BLF = 40kHz
    const float TAG_BIT_D   = 1.0/T_READER_FREQ * pow(10,6); // Duration in us
    const int RN16_D        = (RN16_BITS + TAG_PREAMBLE_BITS) * TAG_BIT_D;
    const int EPC_D          = (EPC_BITS  + TAG_PREAMBLE_BITS) * TAG_BIT_D;
    // Query command 
    const int QUERY_CODE[4] = {1,0,0,0};
    const int M[2]          = {0,0};
    const int SEL[2]         = {0,0};
    const int SESSION[2]     = {0,0};
    const int TARGET         = 0;
    const int TREXT         = 0;
    const int DR            = 0;


    const int NAK_CODE[8]   = {1,1,0,0,0,0,0,0};

    // ACK command
    const int ACK_CODE[2]   = {0,1};
    /* OFDM 200 4-bits */
    const gr_complex yc_cw [] =
    {
     gr_complex(0.0025, 0), gr_complex(0.032, 0.0108), gr_complex(0.0569, 0.0116), gr_complex(0.0552, 0.0063), gr_complex(-0.0704, -0.045), gr_complex(0.0092, -0.036), gr_complex(0.0035, 0.119), gr_complex(0.0315, -0.008), gr_complex(-0.0235, 0.0771), gr_complex(0.0245, 0.0108), gr_complex(-0.053, -0.0125), gr_complex(-0.0171, -0.0253), gr_complex(0.0461, -0.1088), gr_complex(-0.0734, -0.0758), gr_complex(-0.0055, -0.019), gr_complex(0.0384, -0.1647), gr_complex(0.0354, 0.0469), gr_complex(0.0299, 0.0548), gr_complex(0.0509, 0.0644), gr_complex(0.0265, 0.0121), gr_complex(0.011, 0.0256), gr_complex(-0.0493, 0.0197), gr_complex(-0.0317, -0.0191), gr_complex(0.0884, -0.0219), gr_complex(-0.0323, -0.0199), gr_complex(0.0955, -0.0006), gr_complex(0.0029, -0.0091), gr_complex(0.0258, 0.0076), gr_complex(-0.0189, 0.0319), gr_complex(-0.0678, -0.0567), gr_complex(0.0403, -0.0141), gr_complex(0.0216, 0.0583), gr_complex(0.1077, 0.0203), gr_complex(-0.019, 0.0207), gr_complex(-0.0161, 0.0194), gr_complex(0.0041, 0.0501), gr_complex(-0.0085, -0.1039), gr_complex(-0.0182, 0.0549), gr_complex(-0.0137, 0.0331), gr_complex(-0.0724, 0.057), gr_complex(0.0473, 0.0744), gr_complex(-0.0089, -0.0085), gr_complex(0.1169, -0.0037), gr_complex(-0.0449, 0.0338), gr_complex(-0.0456, 0.0938), gr_complex(0.0085, 0.0016), gr_complex(-0.0516, 0.0464), gr_complex(0.0036, 0.042), gr_complex(0.0074, -0.001), gr_complex(0.0297, 0.0522), gr_complex(-0.08, -0.025), gr_complex(0.0337, 0.0069), gr_complex(-0.0021, -0.0881), gr_complex(0.029, -0.1079), gr_complex(-0.0179, 0.0835), gr_complex(-0.0191, 0.0227), gr_complex(-0.039, -0.0467), gr_complex(0.0329, -0.0444), gr_complex(0.0013, -0.0268), gr_complex(-0.0414, -0.0285), gr_complex(0.0165, -0.0224), gr_complex(-0.0174, 0.0436), gr_complex(0.0433, 0.0276), gr_complex(-0.0036, -0.0153), gr_complex(-0.0455, 0.0612), gr_complex(-0.0008, -0.0059), gr_complex(0.0865, -0.0289), gr_complex(0.0769, 0.0388), gr_complex(0.0458, -0.0355), gr_complex(0.0508, 0.0615), gr_complex(-0.0417, -0.0711), gr_complex(0.0563, -0.0551), gr_complex(-0.0193, -0.0313), gr_complex(-0.0838, 0.0217), gr_complex(-0.0168, -0.0965), gr_complex(-0.0955, -0.0206), gr_complex(0.0052, 0.0185), gr_complex(0.0155, 0.0073), gr_complex(-0.1187, -0.0421), gr_complex(-0.034, -0.0302), gr_complex(0.0752, -0.046), gr_complex(-0.0112, -0.0351), gr_complex(-0.0761, 0.0045), gr_complex(-0.0284, 0.0245), gr_complex(0.0229, -0.0503), gr_complex(0.003, -0.0069), gr_complex(-0.1061, -0.0962), gr_complex(0.0242, -0.0068), gr_complex(0.0181, 0.0373), gr_complex(-0.0568, 0.0461), gr_complex(-0.0155, -0.0477), gr_complex(-0.0328, 0.0535), gr_complex(0.0005, 0.0101), gr_complex(0.0608, -0.0508), gr_complex(-0.0405, 0.0438), gr_complex(-0.0433, 0.0647), gr_complex(0.116, 0.0523), gr_complex(0.0213, -0.003), gr_complex(0.0329, -0.0078), gr_complex(-0.0896, -0.128), gr_complex(-0.005, 0), gr_complex(-0.0896, 0.128), gr_complex(0.0329, 0.0078), gr_complex(0.0213, 0.003), gr_complex(0.116, -0.0523), gr_complex(-0.0433, -0.0647), gr_complex(-0.0405, -0.0438), gr_complex(0.0608, 0.0508), gr_complex(0.0005, -0.0101), gr_complex(-0.0328, -0.0535), gr_complex(-0.0155, 0.0477), gr_complex(-0.0568, -0.0461), gr_complex(0.0181, -0.0373), gr_complex(0.0242, 0.0068), gr_complex(-0.1061, 0.0962), gr_complex(0.003, 0.0069), gr_complex(0.0229, 0.0503), gr_complex(-0.0284, -0.0245), gr_complex(-0.0761, -0.0045), gr_complex(-0.0112, 0.0351), gr_complex(0.0752, 0.046), gr_complex(-0.034, 0.0302), gr_complex(-0.1187, 0.0421), gr_complex(0.0155, -0.0073), gr_complex(0.0052, -0.0185), gr_complex(-0.0955, 0.0206), gr_complex(-0.0168, 0.0965), gr_complex(-0.0838, -0.0217), gr_complex(-0.0193, 0.0313), gr_complex(0.0563, 0.0551), gr_complex(-0.0417, 0.0711), gr_complex(0.0508, -0.0615), gr_complex(0.0458, 0.0355), gr_complex(0.0769, -0.0388), gr_complex(0.0865, 0.0289), gr_complex(-0.0008, 0.0059), gr_complex(-0.0455, -0.0612), gr_complex(-0.0036, 0.0153), gr_complex(0.0433, -0.0276), gr_complex(-0.0174, -0.0436), gr_complex(0.0165, 0.0224), gr_complex(-0.0414, 0.0285), gr_complex(0.0013, 0.0268), gr_complex(0.0329, 0.0444), gr_complex(-0.039, 0.0467), gr_complex(-0.0191, -0.0227), gr_complex(-0.0179, -0.0835), gr_complex(0.029, 0.1079), gr_complex(-0.0021, 0.0881), gr_complex(0.0337, -0.0069), gr_complex(-0.08, 0.025), gr_complex(0.0297, -0.0522), gr_complex(0.0074, 0.001), gr_complex(0.0036, -0.042), gr_complex(-0.0516, -0.0464), gr_complex(0.0085, -0.0016), gr_complex(-0.0456, -0.0938), gr_complex(-0.0449, -0.0338), gr_complex(0.1169, 0.0037), gr_complex(-0.0089, 0.0085), gr_complex(0.0473, -0.0744), gr_complex(-0.0724, -0.057), gr_complex(-0.0137, -0.0331), gr_complex(-0.0182, -0.0549), gr_complex(-0.0085, 0.1039), gr_complex(0.0041, -0.0501), gr_complex(-0.0161, -0.0194), gr_complex(-0.019, -0.0207), gr_complex(0.1077, -0.0203), gr_complex(0.0216, -0.0583), gr_complex(0.0403, 0.0141), gr_complex(-0.0678, 0.0567), gr_complex(-0.0189, -0.0319), gr_complex(0.0258, -0.0076), gr_complex(0.0029, 0.0091), gr_complex(0.0955, 0.0006), gr_complex(-0.0323, 0.0199), gr_complex(0.0884, 0.0219), gr_complex(-0.0317, 0.0191), gr_complex(-0.0493, -0.0197), gr_complex(0.011, -0.0256), gr_complex(0.0265, -0.0121), gr_complex(0.0509, -0.0644), gr_complex(0.0299, -0.0548), gr_complex(0.0354, -0.0469), gr_complex(0.0384, 0.1647), gr_complex(-0.0055, 0.019), gr_complex(-0.0734, 0.0758), gr_complex(0.0461, 0.1088), gr_complex(-0.0171, 0.0253), gr_complex(-0.053, 0.0125), gr_complex(0.0245, -0.0108), gr_complex(-0.0235, -0.0771), gr_complex(0.0315, 0.008), gr_complex(0.0035, -0.119), gr_complex(0.0092, 0.036), gr_complex(-0.0704, 0.045), gr_complex(0.0552, -0.0063), gr_complex(0.0569, -0.0116), gr_complex(0.016, -0.0054)
    };
    // /*OFDM 20 1-bits*/
    //  const gr_complex yc_cw [] =
    //  {
    //   gr_complex(-0.1, 0), gr_complex(0.0357, -0.1155), gr_complex(-0.295, -0.0294), gr_complex(0.2412, 0.0595), gr_complex(-0.0964, -0.0476), gr_complex(-0.15, -0.15), gr_complex(0.32, -0.0476), gr_complex(0.1824, 0.0595), gr_complex(0.0714, -0.0294), gr_complex(-0.0594, -0.1155), gr_complex(-0.3, 0), gr_complex(-0.0594, 0.1155), gr_complex(0.0714, 0.0294), gr_complex(0.1824, -0.0595), gr_complex(0.32, 0.0476), gr_complex(-0.15, 0.15), gr_complex(-0.0964, 0.0476), gr_complex(0.2412, -0.0595), gr_complex(-0.295, 0.0294), gr_complex(0.01785, 0.05775)
    //  };

    // /* OFDM 20 1-bits */
    //  const gr_complex yc_cw [] =
    //  {
    //   gr_complex(-0.1, 0), gr_complex(0.0357, -0.1155), gr_complex(-0.295, -0.0294), gr_complex(0.2412, 0.0595), gr_complex(-0.0964, -0.0476), gr_complex(-0.15, -0.15), gr_complex(0.32, -0.0476), gr_complex(0.1824, 0.0595), gr_complex(0.0714, -0.0294), gr_complex(-0.0594, -0.1155), gr_complex(-0.3, 0), gr_complex(-0.0594, 0.1155), gr_complex(0.0714, 0.0294), gr_complex(0.1824, -0.0595), gr_complex(0.32, 0.0476), gr_complex(-0.15, 0.15), gr_complex(-0.0964, 0.0476), gr_complex(0.2412, -0.0595), gr_complex(-0.295, 0.0294), gr_complex(0.01785, 0.05775)
    //  };

    // /* FMCW 200-4bits */
    // const gr_complex yc_cw [] =
    // {
    //    gr_complex(1, 0), gr_complex(1, 0.0015866), gr_complex(0.99998, 0.0063464), gr_complex(0.9999, 0.014279), gr_complex(0.99968, 0.025383), gr_complex(0.99921, 0.039655), gr_complex(0.99837, 0.057087), gr_complex(0.99698, 0.077666), gr_complex(0.99485, 0.10137), gr_complex(0.99175, 0.12816), gr_complex(0.98744, 0.158), gr_complex(0.98163, 0.1908), gr_complex(0.97401, 0.22649), gr_complex(0.96427, 0.26494), gr_complex(0.95203, 0.30599), gr_complex(0.93695, 0.34946), gr_complex(0.91864, 0.3951), gr_complex(0.8967, 0.44263), gr_complex(0.87075, 0.49172), gr_complex(0.8404, 0.54196), gr_complex(0.80528, 0.5929), gr_complex(0.76503, 0.64399), gr_complex(0.71935, 0.69464), gr_complex(0.66797, 0.74419), gr_complex(0.61067, 0.79189), gr_complex(0.54732, 0.83692), gr_complex(0.47788, 0.87843), gr_complex(0.40241, 0.91546), gr_complex(0.32109, 0.94705), gr_complex(0.23425, 0.97218), gr_complex(0.14235, 0.98982), gr_complex(0.046035, 0.99894), gr_complex(-0.053879, 0.99855), gr_complex(-0.15639, 0.9877), gr_complex(-0.26031, 0.96553), gr_complex(-0.36424, 0.93131), gr_complex(-0.46662, 0.88446), gr_complex(-0.56571, 0.82461), gr_complex(-0.6596, 0.75162), gr_complex(-0.74628, 0.66563), gr_complex(-0.82364, 0.56711), gr_complex(-0.88953, 0.45688), gr_complex(-0.94182, 0.33612), gr_complex(-0.97846, 0.20643), gr_complex(-0.99756, 0.069834), gr_complex(-0.99746, -0.071258), gr_complex(-0.97683, -0.21403), gr_complex(-0.93474, -0.35532), gr_complex(-0.87079, -0.49165), gr_complex(-0.78513, -0.61933), gr_complex(-0.67858, -0.73452), gr_complex(-0.55269, -0.83339), gr_complex(-0.40973, -0.91221), gr_complex(-0.25279, -0.96752), gr_complex(-0.085691, -0.99632), gr_complex(0.087035, -0.99621), gr_complex(0.26023, -0.96555), gr_complex(0.42824, -0.90366), gr_complex(0.58511, -0.81096), gr_complex(0.72473, -0.68903), gr_complex(0.84118, -0.54076), gr_complex(0.9289, -0.37033), gr_complex(0.98308, -0.18317), gr_complex(0.9999, 0.01412), gr_complex(0.97684, 0.21396), gr_complex(0.91297, 0.40803), gr_complex(0.80912, 0.58764), gr_complex(0.66808, 0.74409), gr_complex(0.49465, 0.86909), gr_complex(0.29559, 0.95532), gr_complex(0.079446, 0.99684), gr_complex(-0.14369, 0.98962), gr_complex(-0.36261, 0.93194), gr_complex(-0.56558, 0.8247), gr_complex(-0.74087, 0.67165), gr_complex(-0.87757, 0.47945), gr_complex(-0.96628, 0.25751), gr_complex(-0.99984, 0.01769), gr_complex(-0.97407, -0.22626), gr_complex(-0.88822, -0.45941), gr_complex(-0.74544, -0.66658), gr_complex(-0.55282, -0.8333), gr_complex(-0.32132, -0.94697), gr_complex(-0.065283, -0.99787), gr_complex(0.19831, -0.98014), gr_complex(0.4509, -0.89257), gr_complex(0.67361, -0.73908), gr_complex(0.84873, -0.52883), gr_complex(0.96118, -0.27594), gr_complex(1, 0.0012693), gr_complex(0.95958, 0.28142), gr_complex(0.84057, 0.5417), gr_complex(0.65031, 0.75967), gr_complex(0.4027, 0.91533), gr_complex(0.1175, 0.99307), gr_complex(-0.1811, 0.98346), gr_complex(-0.46634, 0.88461), gr_complex(-0.71132, 0.70287), gr_complex(-0.89155, 0.45292), gr_complex(-0.98763, 0.15682), gr_complex(-0.9875, -0.15761), gr_complex(-0.8883, -0.45927), gr_complex(-0.69724, -0.71684), gr_complex(-0.43161, -0.90206), gr_complex(-0.11757, -0.99306), gr_complex(0.21213, -0.97724), gr_complex(0.52144, -0.85329), gr_complex(0.77486, -0.63214), gr_complex(0.94169, -0.33649), gr_complex(1, 0.0011106), gr_complex(0.93986, 0.34156), gr_complex(0.76534, 0.64362), gr_complex(0.49493, 0.86893), gr_complex(0.16007, 0.98711), gr_complex(-0.19808, 0.98019), gr_complex(-0.5335, 0.8458), gr_complex(-0.80117, 0.59844), gr_complex(-0.96327, 0.26853), gr_complex(-0.9949, -0.10082), gr_complex(-0.88837, -0.45913), gr_complex(-0.6553, -0.75537), gr_complex(-0.32612, -0.94533), gr_complex(0.053325, -0.99858), gr_complex(0.42781, -0.90387), gr_complex(0.7406, -0.67194), gr_complex(0.94217, -0.33514), gr_complex(0.99841, 0.056454), gr_complex(0.89698, 0.44206), gr_complex(0.65055, 0.75946), gr_complex(0.29604, 0.95517), gr_complex(-0.11017, 0.99391), gr_complex(-0.50078, 0.86558), gr_complex(-0.80861, 0.58835), gr_complex(-0.97833, 0.20705), gr_complex(-0.97696, -0.21341), gr_complex(-0.80098, -0.59869), gr_complex(-0.47851, -0.87808), gr_complex(-0.065758, -0.99784), gr_complex(0.3621, -0.93214), gr_complex(0.72429, -0.68949), gr_complex(0.94983, -0.31278), gr_complex(0.99185, 0.12738), gr_complex(0.83825, 0.54529), gr_complex(0.51574, 0.85675), gr_complex(0.086403, 0.99626), gr_complex(-0.3635, 0.93159), gr_complex(-0.74044, 0.67212), gr_complex(-0.96319, 0.26884), gr_complex(-0.98088, -0.19462), gr_complex(-0.78562, -0.61871), gr_complex(-0.41623, -0.90926), gr_complex(0.048254, -0.99884), gr_complex(0.50469, -0.8633), gr_complex(0.84839, -0.52937), gr_complex(0.9975, -0.070704), gr_complex(0.91329, 0.40731), gr_complex(0.61142, 0.79131), gr_complex(0.16054, 0.98703), gr_complex(-0.33212, 0.94324), gr_complex(-0.74565, 0.66634), gr_complex(-0.9752, 0.22131), gr_complex(-0.95934, -0.28226), gr_complex(-0.69769, -0.7164), gr_complex(-0.25371, -0.96728), gr_complex(0.25931, -0.96579), gr_complex(0.70634, -0.70788), gr_complex(0.96605, -0.25835), gr_complex(0.96456, 0.26387), gr_complex(0.69775, 0.71634), gr_complex(0.23533, 0.97192), gr_complex(-0.29581, 0.95525), gr_complex(-0.74554, 0.66646), gr_complex(-0.98289, 0.18418), gr_complex(-0.93514, -0.35428), gr_complex(-0.61161, -0.79116), gr_complex(-0.10417, -0.99456), gr_complex(0.43726, -0.89934), gr_complex(0.84822, -0.52964), gr_complex(0.99999, 0.0050772), gr_complex(0.84109, 0.5409), gr_complex(0.4166, 0.90909), gr_complex(-0.14259, 0.98978), gr_complex(-0.65865, 0.75245), gr_complex(-0.96306, 0.2693), gr_complex(-0.95245, -0.30471), gr_complex(-0.62545, -0.78027), gr_complex(-0.086956, -0.99621), gr_complex(0.48358, -0.8753), gr_complex(0.89108, -0.45384), gr_complex(0.99194, 0.12675), gr_complex(0.74623, 0.66569), gr_complex(0.23564, 0.97184), gr_complex(-0.36143, 0.9324), gr_complex(-0.83086, 0.55648), gr_complex(-0.99971, -0.023876), gr_complex(-0.80145, -0.59806), gr_complex(-0.30444, -0.95253), gr_complex(0.30905, -0.95105), gr_complex(0.80809, -0.58906), gr_complex(1, -2.4493e-15)

    // };
   
    // QueryAdjust command
    const int QADJ_CODE[4]   = {1,0,0,1};

    // 110 Increment by 1, 000 unchanged, 011 decrement by 1
    const int Q_UPDN[3][3]  = { {1,1,0}, {0,0,0}, {0,1,1} };

    // FM0 encoding preamble sequences
    const int TAG_PREAMBLE[] = {1,1,0,1,0,0,1,0,0,0,1,1};

    // Gate block parameters
    const float THRESH_FRACTION = 0.75;     
    const int WIN_SIZE_D         = 250; 

    // Duration in which dc offset is estimated (T1_D is 250)
    const int DC_SIZE_D         = 120;

    // Global variable
    extern READER_STATE * reader_state;
    extern void initialize_reader_state();

  } // namespace rfid
} // namespace gr

#endif /* INCLUDED_RFID_GLOBAL_VARS_H */

