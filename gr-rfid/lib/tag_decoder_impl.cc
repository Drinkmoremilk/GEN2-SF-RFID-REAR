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
/* Modified by Yuchen Jiang, SUSTech */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <gnuradio/prefs.h>
#include <gnuradio/math.h>
#include <cmath>
#include <sys/time.h>
#include "tag_decoder_impl.h"
#include "rfid/global_vars.h"

namespace gr {
  namespace rfid {

    tag_decoder::sptr
    tag_decoder::make (int sample_rate)
    {
      std::vector<int> input_sizes;
      input_sizes.push_back(sizeof(gr_complex));
      input_sizes.push_back(sizeof(gr_complex));

      std::vector<int> output_sizes;
      output_sizes.push_back(sizeof(float));
      output_sizes.push_back(sizeof(gr_complex));
      return gnuradio::get_initial_sptr
        (new tag_decoder_impl(sample_rate, input_sizes, output_sizes));
    }

    /*
     * The private constructor
     */
    tag_decoder_impl::tag_decoder_impl(int sample_rate,std::vector<int> input_sizes, std::vector<int> output_sizes)
      : gr::block("tag_decoder",
              gr::io_signature::makev(2, 2, input_sizes),
              gr::io_signature::makev(2, 2, output_sizes )),
              s_rate(sample_rate)
    {

      char_bits = (char *) malloc( sizeof(char) * 128);

      n_samples_TAG_BIT = TAG_BIT_D * s_rate / pow(10,6);      
      GR_LOG_INFO(d_logger, "Number of samples of Tag bit : "<< n_samples_TAG_BIT);
    }

    /*
     * Our virtual destructor.
     */
    tag_decoder_impl::~tag_decoder_impl()
    {

    }

    void
    tag_decoder_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
        ninput_items_required[0] = noutput_items;
    }

    int tag_decoder_impl::tag_sync(const gr_complex * in , const gr_complex * in_RN16_EPC, int size)
    {
      int max_index = 0;
      float max = 0,corr;
      gr_complex corr2;
      
      // Do not have to check entire vector (not optimal)
      for (int i= 5*n_samples_TAG_BIT; i < 6*n_samples_TAG_BIT; i++)
      {
        corr2 = gr_complex(0,0);
        corr = 0;
        // sync after matched filter (equivalent)
        for (int j = 0; j <2* TAG_PREAMBLE_BITS; j ++)
        {
          corr2 = corr2 + in[ (int) (i+j*n_samples_TAG_BIT/2) ] * gr_complex(TAG_PREAMBLE[j],0);
        }
        corr = std::norm(corr2);
        if (corr > max)
        {
          max = corr;
          max_index = i;
        }
      }  
      //using preamble to estimate the high and low level of FM0(Preamble ({1,1,-1,1,-1,-1,1,-1,-1,-1,1,1} 1 2 4 7 11 12))) 
      h_est_h = (in_RN16_EPC[max_index] + in_RN16_EPC[ (int) (max_index + n_samples_TAG_BIT/2) ] + in_RN16_EPC[ (int) (max_index + 3*n_samples_TAG_BIT/2) ] + in_RN16_EPC[ (int) (max_index + 6*n_samples_TAG_BIT/2)] + in_RN16_EPC[(int) (max_index + 10*n_samples_TAG_BIT/2) ] + in_RN16_EPC[ (int) (max_index + 11*n_samples_TAG_BIT/2)])/std::complex<float>(6,0);  
      h_est_l = (in_RN16_EPC[(int) (max_index + n_samples_TAG_BIT)] + in_RN16_EPC[ (int) (max_index + 2*n_samples_TAG_BIT) ] + in_RN16_EPC[ (int) (max_index + 5*n_samples_TAG_BIT/2) ] + in_RN16_EPC[ (int) (max_index + 7*n_samples_TAG_BIT/2)] + in_RN16_EPC[(int) (max_index + 4*n_samples_TAG_BIT) ] + in_RN16_EPC[ (int) (max_index + 9*n_samples_TAG_BIT/2)])/std::complex<float>(6,0);  
      
      // Shifted received waveform by n_samples_TAG_BIT/2
      max_index = max_index + TAG_PREAMBLE_BITS * n_samples_TAG_BIT + n_samples_TAG_BIT/2; 
      return max_index;  
    }



    std::vector<float>  tag_decoder_impl::tag_detection_RN16(std::vector<gr_complex> & RN16_samples_complex)
    {
      // detection + differential decoder (since Tag uses FM0)
      std::vector<float> tag_bits,dist;
      float result;
      int prev = 1,index_T=0;
      
      for (int j = 0; j < RN16_samples_complex.size()/2 ; j ++ )
      {
        result = std::real( (RN16_samples_complex[2*j] - RN16_samples_complex[2*j+1])*std::conj(h_est_h)); 
  
        if (result>0){
          if (prev == 1)
            tag_bits.push_back(0);
          else
            tag_bits.push_back(1);      
          prev = 1;      
        }
        else
        { 
          if (prev == -1)
            tag_bits.push_back(0);
          else
            tag_bits.push_back(1);      
          prev = -1;    
        }
      }
      return tag_bits;
    }


    std::vector<float>  tag_decoder_impl::tag_detection_EPC(std::vector<gr_complex> & EPC_samples_complex,int index)
    {
      std::vector<float> tag_bits,dist;
      gr_complex h_e_h = 0;
      gr_complex h_e_l = 0;
      gr_complex h_e = 0;
      float result=0;
      int prev = 1;
      
      int number_steps = 20;
      float min_val = n_samples_TAG_BIT/2.0 -  n_samples_TAG_BIT/2.0/100, max_val = n_samples_TAG_BIT/2.0 +  n_samples_TAG_BIT/2.0/100;

      std::vector<float> energy;
      energy.resize(number_steps);
      for (int t = 0; t <number_steps; t++)
      {  
        for (int i =0; i <256; i++)
        {
          energy[t]+= reader_state->magn_squared_samples[(int) (i * (min_val + t*(max_val-min_val)/(number_steps-1)) + index)];
        }

      }
      int index_T = std::distance(energy.begin(), std::max_element(energy.begin(), energy.end()));
      float T =  min_val + index_T*(max_val-min_val)/(number_steps-1);
      
      // T estimated
      T_global = T;
      

      for (int j = 0; j < 128 ; j ++ )
      {
        result = std::real((EPC_samples_complex[ (int) (j*(2*T) + index) ] - EPC_samples_complex[ (int) (j*2*T + T + index) ])*std::conj(h_est_h) ); 

         if (result>0){
         
          if (prev == 1) 
            tag_bits.push_back(0);
          else
            tag_bits.push_back(1);      
          prev = 1;      
        }
        else
        {
          
          if (prev == -1)
            tag_bits.push_back(0);
          else
            tag_bits.push_back(1);      
          prev = -1;    
        }
      }
      return tag_bits;
    }


    int
    tag_decoder_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const  gr_complex *) input_items[0];
      const gr_complex *in_RN16_EPC = (const  gr_complex *) input_items[1]; // for EPC loading
      float *out = (float *) output_items[0];
      gr_complex *out_2 = (gr_complex *) output_items[1]; // for EPC loading
      int written_sync =0;
      int written = 0, consumed = 0;

      int RN16_index , EPC_index;

      
      std::vector<float> RN16_samples_real;
      std::vector<float> EPC_samples_real;
      
      std::vector<gr_complex> RN16_samples_complex;
      std::vector<gr_complex> EPC_samples_complex;
      std::vector<gr_complex> RAW_EPC_samples, RAW_RN16_samples;

      std::vector<float> RN16_bits;
      int number_of_half_bits = 0;

      std::vector<float> EPC_bits;    
      // Processing only after n_samples_to_ungate are available and we need to decode an RN16
      if (reader_state->decoder_status == DECODER_DECODE_RN16 && ninput_items[0] >= reader_state->n_samples_to_ungate)
      {
        RN16_index = tag_sync(in,in_RN16_EPC , ninput_items[0]);
        /* When RN16 positioning is complete, simultaneously capture the high and low amplitude levels of the FM0 signa */
        if (reader_state->reader_stats.h_flag)
        {
          reader_state->reader_stats.h_flag = 0;

          /*caculate the ratio */
          reader_state->reader_stats.h_h = std::abs(h_est_h);
          reader_state->reader_stats.h_l  = std::abs( h_est_l);
          reader_state->reader_stats.scale = 5*(std::abs((reader_state->reader_stats.h_h-reader_state->reader_stats.h_l)))/reader_state->reader_stats.h_l;
        }
        // std::cout<<"index"<<RN16_index<<std::endl;
        for (float j = RN16_index ; j < ninput_items[0]; j += n_samples_TAG_BIT/2 )
        {
          number_of_half_bits++;
          int k = round(j);
          RN16_samples_complex.push_back(in[k]);
          
          if (number_of_half_bits == 2*(RN16_BITS-1))
          {
            break;
          }
        }    

        // RN16 bits are passed to the next block for the creation of ACK message
        if (number_of_half_bits == 2*(RN16_BITS-1))
        {  
          GR_LOG_INFO(d_debug_logger, "RN16 DECODED");
          RN16_bits  = tag_detection_RN16(RN16_samples_complex);
          for(int bit=0; bit<RN16_bits.size(); bit++)
          {
            out[written] =  RN16_bits[bit];
            written ++;
          }
          produce(0,written);
          reader_state->gen2_logic_status = SEND_ACK;
        }
        else
        {  
          reader_state->reader_stats.cur_slot_number++;
          if(reader_state->reader_stats.cur_slot_number > reader_state->reader_stats.max_slot_number)
          {
            reader_state->reader_stats.cur_slot_number = 1;
            reader_state->reader_stats.unique_tags_round.push_back(reader_state->reader_stats.tag_reads.size());

            reader_state->reader_stats.cur_inventory_round += 1;
    
            //if (P_DOWN == true)
            //  reader_state->gen2_logic_status = POWER_DOWN;
            //else
              reader_state->gen2_logic_status = SEND_QUERY;
          }
          else
          {
            reader_state->gen2_logic_status = SEND_QUERY_REP;
          }
        }
        consumed = reader_state->n_samples_to_ungate;
      }
      else if (reader_state->decoder_status == DECODER_DECODE_EPC && ninput_items[0] >= reader_state->n_samples_to_ungate )
      {  

        //After EPC message send a query rep or query
        reader_state->reader_stats.cur_slot_number++;
        EPC_index = tag_sync(in,in_RN16_EPC,ninput_items[0]);

        for (int j = 0; j < ninput_items[0]; j++ )
        {
          EPC_samples_complex.push_back(in[j]);
          // EPC_samples_complex.push_back(in_RN16_EPC[j]);
          // if (j<50)
          // {
          //   RAW_EPC_samples.push_back(in_RN16_EPC[j]);
          // }
          // EPC_samples_complex[j] = EPC_samples_complex[j]-RAW_EPC_samples[j%50]; // cancellation    
        }
        for (int j = 0; j < ninput_items[1] ; j ++ )
        {
          out_2[written_sync] = in_RN16_EPC[j]; //offline decoding
          // out_2[written_sync] = EPC_samples_complex[j]; //check cancellation
          written_sync ++;          
        }
        produce(1,written_sync);
        
        
        
        // std::cout<<T_global<<std::endl;
        
        EPC_bits   = tag_detection_EPC(EPC_samples_complex, EPC_index);
        if (EPC_bits.size() == EPC_BITS - 1)
        {
          // float to char -> use Buettner's function
          for (int i =0; i < 128; i ++)
          {
            if (EPC_bits[i] == 0)
              char_bits[i] = '0';
            else
              char_bits[i] = '1';
          }
          if(check_crc(char_bits,128) == 1)
          {
            if(reader_state->reader_stats.cur_slot_number > reader_state->reader_stats.max_slot_number)
            {
              reader_state->reader_stats.cur_slot_number = 1;
              reader_state->reader_stats.unique_tags_round.push_back(reader_state->reader_stats.tag_reads.size());
        
              reader_state->reader_stats.cur_inventory_round+=1;
              //if (P_DOWN == true)
              //  reader_state->gen2_logic_status = POWER_DOWN;
              //else
                reader_state->gen2_logic_status = SEND_QUERY;
            }
            else
            {
              reader_state->gen2_logic_status = SEND_QUERY_REP;
            }
            // method NO.1
            
            reader_state->reader_stats.n_epc_correct+=1;
            int result = 0;
            for(int i = 0 ; i < 8 ; ++i)
            {
              result += std::pow(2,7-i) * EPC_bits[104+i] ;
            }
            GR_LOG_INFO(d_debug_logger, "EPC CORRECTLY DECODED, TAG ID : " << result);
            // Save part of Tag's EPC message (EPC[104:111] in decimal) + number of reads
            std::map<int,int>::iterator it = reader_state->reader_stats.tag_reads.find(result);
            if ( it != reader_state->reader_stats.tag_reads.end())
            {
              it->second ++;
            }
            else
            {
              reader_state->reader_stats.tag_reads[result]=1;
            }

          }
          else
          {     

            if(reader_state->reader_stats.cur_slot_number > reader_state->reader_stats.max_slot_number)
            {
              reader_state->reader_stats.cur_slot_number = 1;
              reader_state->reader_stats.cur_inventory_round+=1;
              //if (P_DOWN == true)
              //  reader_state->gen2_logic_status = POWER_DOWN;
              //else
              //  reader_state->gen2_logic_status = SEND_NAK_Q;
                reader_state->gen2_logic_status = SEND_QUERY;
            }
            else
            {
                //reader_state->gen2_logic_status = SEND_NAK_QR;
                reader_state->gen2_logic_status = SEND_QUERY_REP;
            }

            
            GR_LOG_INFO(d_debug_logger, "EPC FAIL TO DECODE");  
          }
        }
        else
        {
          GR_LOG_EMERG(d_debug_logger, "CHECK ME");  
        }
        consumed = reader_state->n_samples_to_ungate;
      }
      consume_each(consumed);
      return WORK_CALLED_PRODUCE;
    }


    /* Function adapted from https://www.cgran.org/wiki/Gen2 */
    int tag_decoder_impl::check_crc(char * bits, int num_bits)
    {
      register unsigned short i, j;
      register unsigned short crc_16, rcvd_crc;
      unsigned char * data;
      int num_bytes = num_bits / 8;
      data = (unsigned char* )malloc(num_bytes );
      int mask;

      for(i = 0; i < num_bytes; i++)
      {
        mask = 0x80;
        data[i] = 0;
        for(j = 0; j < 8; j++)
        {
          if (bits[(i * 8) + j] == '1'){
          data[i] = data[i] | mask;
        }
        mask = mask >> 1;
        }
      }
      rcvd_crc = (data[num_bytes - 2] << 8) + data[num_bytes -1];

      crc_16 = 0xFFFF; 
      for (i=0; i < num_bytes - 2; i++)
      {
        crc_16^=data[i] << 8;
        for (j=0;j<8;j++)
        {
          if (crc_16&0x8000)
          {
            crc_16 <<= 1;
            crc_16 ^= 0x1021;
          }
          else
            crc_16 <<= 1;
        }
      }
      crc_16 = ~crc_16;

      if(rcvd_crc != crc_16)
        return -1;
      else
        return 1;
    }
  } /* namespace rfid */
} /* namespace gr */

