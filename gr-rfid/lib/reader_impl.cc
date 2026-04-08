/* -*- c++ -*- */
/* /* Modyfied by Yuchen Jiang
 * Copyright 2015 <Nikos Kargas (nkargas@isc.tuc.gr)>. 
 * * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version. 
 * * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. 
 * * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "reader_impl.h"
#include "gnuradio/rfid/global_vars.h"
#include <sys/time.h>
#include <algorithm> // 添加 algorithm 库以支持 std::min

namespace gr {
  namespace rfid {

    reader::sptr
    reader::make(int sample_rate, int dac_rate)
    {
      return gnuradio::get_initial_sptr
        (new reader_impl(sample_rate,dac_rate));
    }

    /*
     * The private constructor
     */
    reader_impl::reader_impl(int sample_rate, int dac_rate)
      : gr::block("reader",
              gr::io_signature::make( 1, 1, sizeof(float)),       // 输入保持 float (接收解码器的 0/1 位流)
              gr::io_signature::make( 1, 1, sizeof(gr_complex))), // [修改] 输出改为复数 gr_complex (发送基带 IQ 信号)
        d_tx_index(0) // 初始化发送索引
    {

      //d_logger->info( "Block initialized");

      sample_d = 1.0/dac_rate * pow(10,6);

      // Number of samples for transmitting

      n_data0_s = 2 * PW_D / sample_d;
      n_data1_s = 4 * PW_D / sample_d;
      n_pw_s    = PW_D    / sample_d;
      n_cw_s    = CW_D    / sample_d;
      n_delim_s = DELIM_D / sample_d;
      n_trcal_s = TRCAL_D / sample_d;

      d_logger->info( "Number of samples data 0 : {}", n_data0_s);
      d_logger->info( "Number of samples data 1 : {}", n_data1_s);
      d_logger->info( "Number of samples cw : {}",     n_cw_s);
      d_logger->info( "Number of samples delim : {}",  n_delim_s);
      d_logger->info( "Number of slots : {}",          std::pow(2,FIXED_Q));

      // CW waveforms of different sizes
      n_cwquery_s   = (T1_D+T2_D+RN16_D)/sample_d;     //RN16
      n_cwack_s     = (3*T1_D+T2_D+EPC_D)/sample_d;    //EPC   if it is longer than nominal it wont cause tags to change inventoried flag
      n_p_down_s     = (P_DOWN_D)/sample_d;  

      p_down.resize(n_p_down_s);        // Power down samples
      cw_query.resize(n_cwquery_s);      // Sent after query/query rep
      cw_ack.resize(n_cwack_s);          // Sent after ack

      // [修改] 填充为复数 1.0 + j0.0
      std::fill_n(cw_query.begin(), cw_query.size(), gr_complex(1.0, 0.0));
      std::fill_n(cw_ack.begin(), cw_ack.size(), gr_complex(1.0, 0.0));


      // ==========================================
      // 【修改】：读取 MATLAB 生成的复数波形并循环拼接
      // ==========================================
      std::vector<gr_complex> AugCW_base; // [修改] 存放复数
      std::ifstream infile("/home/yelab/Desktop/jyc/Gen2-UHF-RFID-Reader_AugCW_adaptive_ADC/augcw_sar_-5dB.txt"); 
      
      if (infile.is_open()) {
          float real_val, imag_val;
          // [修改] 每次读取两个数字（实部和虚部）
          while (infile >> real_val >> imag_val) {
              AugCW_base.push_back(gr_complex(real_val, imag_val));
          }
          infile.close();

          if (!AugCW_base.empty()) {
              int repeat_count = n_cwack_s / AugCW_base.size();
              int remainder = n_cwack_s % AugCW_base.size(); 

              for (int i = 0; i < repeat_count; ++i) {
                  AugCW.insert(AugCW.end(), AugCW_base.begin(), AugCW_base.end());
              }
              AugCW.insert(AugCW.end(), AugCW_base.begin(), AugCW_base.begin() + remainder);

              d_logger->info("成功加载并拼接 OFDM AugCW！基础波形长度: {}, 最终发送长度: {}", AugCW_base.size(), AugCW.size());
          } else {
              d_logger->error("波形文件为空！将退回使用默认全 1 载波。");
              AugCW = cw_ack; 
          }
      } else {
          d_logger->error("读取自定义波形文件失败！将退回使用默认全 1 载波。");
          AugCW = cw_ack; 
      }

      // Construct vectors (resize() default initialization is zero)
      data_0.resize(n_data0_s);
      data_1.resize(n_data1_s);
      cw.resize(n_cw_s);
      delim.resize(n_delim_s);
      rtcal.resize(n_data0_s + n_data1_s);
      trcal.resize(n_trcal_s);

      // [修改] 填充为复数 1.0 + j0.0
      std::fill_n(data_0.begin(), data_0.size()/2, gr_complex(1.0, 0.0));
      std::fill_n(data_1.begin(), 3*data_1.size()/4, gr_complex(1.0, 0.0));
      std::fill_n(cw.begin(), cw.size(), gr_complex(1.0, 0.0));
      std::fill_n(rtcal.begin(), rtcal.size() - n_pw_s, gr_complex(1.0, 0.0)); 
      std::fill_n(trcal.begin(), trcal.size() - n_pw_s, gr_complex(1.0, 0.0)); 

      // create preamble
      preamble.insert( preamble.end(), delim.begin(), delim.end() );
      preamble.insert( preamble.end(), data_0.begin(), data_0.end() );
      preamble.insert( preamble.end(), rtcal.begin(), rtcal.end() );
      preamble.insert( preamble.end(), trcal.begin(), trcal.end() );

      // create framesync
      frame_sync.insert( frame_sync.end(), delim.begin() , delim.end() );
      frame_sync.insert( frame_sync.end(), data_0.begin(), data_0.end() );
      frame_sync.insert( frame_sync.end(), rtcal.begin() , rtcal.end() );
      
      // create query rep
      query_rep.insert( query_rep.end(), frame_sync.begin(), frame_sync.end());
      query_rep.insert( query_rep.end(), data_0.begin(), data_0.end() );
      query_rep.insert( query_rep.end(), data_0.begin(), data_0.end() );
      query_rep.insert( query_rep.end(), data_0.begin(), data_0.end() );
      query_rep.insert( query_rep.end(), data_0.begin(), data_0.end() );

      // create nak
      nak.insert( nak.end(), frame_sync.begin(), frame_sync.end());
      nak.insert( nak.end(), data_1.begin(), data_1.end() );
      nak.insert( nak.end(), data_1.begin(), data_1.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );

      gen_query_bits();
      gen_query_adjust_bits();
    }

    void reader_impl::gen_query_bits()
    {
      int num_ones = 0, num_zeros = 0;

      query_bits.resize(0);
      query_bits.insert(query_bits.end(), &QUERY_CODE[0], &QUERY_CODE[4]);
      query_bits.push_back(DR);
      query_bits.insert(query_bits.end(), &M[0], &M[2]);
      query_bits.push_back(TREXT);
      query_bits.insert(query_bits.end(), &SEL[0], &SEL[2]);
      query_bits.insert(query_bits.end(), &SESSION[0], &SESSION[2]);
      query_bits.push_back(TARGET);
    
      query_bits.insert(query_bits.end(), &Q_VALUE[FIXED_Q][0], &Q_VALUE[FIXED_Q][4]);
      crc_append(query_bits);
    }


    void reader_impl::gen_ack_bits(const float * in)
    {
      ack_bits.resize(0);
      ack_bits.insert(ack_bits.end(), &ACK_CODE[0], &ACK_CODE[2]);
      ack_bits.insert(ack_bits.end(), &in[0], &in[16]);
    }
  
    void reader_impl::gen_query_adjust_bits()
    {
      query_adjust_bits.resize(0);
      query_adjust_bits.insert(query_adjust_bits.end(), &QADJ_CODE[0], &QADJ_CODE[4]);
      query_adjust_bits.insert(query_adjust_bits.end(), &SESSION[0], &SESSION[2]);
      query_adjust_bits.insert(query_adjust_bits.end(), &Q_UPDN[1][0], &Q_UPDN[1][3]);
    }


    /*
     * Our virtual destructor.
     */
    reader_impl::~reader_impl()
    {

    }

    void reader_impl::print_results()
    {
      std::cout << "\n --------------------------" << std::endl;
      std::cout << "| Number of queries/queryreps sent : " << reader_state->reader_stats.n_queries_sent - 1 << std::endl;
      std::cout << "| Current Inventory round : "         << reader_state->reader_stats.cur_inventory_round << std::endl;
      std::cout << " --------------------------"            << std::endl;

      std::cout << "| Correctly decoded EPC : "  <<  reader_state->reader_stats.n_epc_correct     << std::endl;
      std::cout << "| Number of unique tags : "  <<  reader_state->reader_stats.tag_reads.size() << std::endl;

      std::map<int,int>::iterator it;

      for(it = reader_state->reader_stats.tag_reads.begin(); it != reader_state->reader_stats.tag_reads.end(); it++) 
      {
        std::cout << std::hex <<  "| Tag ID : " << it->first << "  ";
        std::cout << "Num of reads : " << std::dec << it->second << std::endl;
      }

      std::cout << " --------------------------" << std::endl;
    }

    void
    reader_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = 0;
    }

    int
    reader_impl::general_work (int noutput_items,
                               gr_vector_int &ninput_items,
                               gr_vector_const_void_star &input_items,
                               gr_vector_void_star &output_items)
    {

      const float *in = (const float *) input_items[0];
      gr_complex *out = (gr_complex*) output_items[0]; // [修改] 强制转换为 gr_complex* 指针
      int consumed = 0;

      // ==========================================
      // 阶段 1：发送缓冲区中剩余的数据
      // ==========================================
      if (d_tx_index < d_tx_buffer.size()) {
          int items_to_send = std::min((size_t)noutput_items, d_tx_buffer.size() - d_tx_index);
          
          // [修改] sizeof 改为 gr_complex
          memcpy(out, &d_tx_buffer[d_tx_index], items_to_send * sizeof(gr_complex));
          
          d_tx_index += items_to_send;
          
          consume_each(0); 
          return items_to_send; 
      }

      // ==========================================
      // 阶段 2：清空缓冲区，准备生成新波形
      // ==========================================
      d_tx_buffer.clear();
      d_tx_index = 0;
      consumed = ninput_items[0]; 

      // ==========================================
      // 阶段 3：状态机运行，将波形追加到 d_tx_buffer
      // ==========================================
      switch (reader_state->gen2_logic_status)
      {
        case START:
          d_tx_buffer.insert(d_tx_buffer.end(), cw_ack.begin(), cw_ack.end());
          reader_state->gen2_logic_status = SEND_QUERY;    
          break;

        case POWER_DOWN:
          d_tx_buffer.insert(d_tx_buffer.end(), p_down.begin(), p_down.end());
          reader_state->gen2_logic_status = START;    
          break;

        case SEND_NAK_QR:
          d_tx_buffer.insert(d_tx_buffer.end(), nak.begin(), nak.end());
          d_tx_buffer.insert(d_tx_buffer.end(), cw.begin(), cw.end());
          reader_state->gen2_logic_status = SEND_QUERY_REP;    
          break;

        case SEND_NAK_Q:
          d_tx_buffer.insert(d_tx_buffer.end(), nak.begin(), nak.end());
          d_tx_buffer.insert(d_tx_buffer.end(), cw.begin(), cw.end());
          reader_state->gen2_logic_status = SEND_QUERY;    
          break;

        case SEND_QUERY:
          reader_state->reader_stats.n_queries_sent +=1;  
          reader_state->decoder_status = DECODER_DECODE_RN16;
          reader_state->gate_status    = GATE_SEEK_RN16;

          d_tx_buffer.insert(d_tx_buffer.end(), preamble.begin(), preamble.end());
   
          for(int i = 0; i < query_bits.size(); i++)
          {
            if(query_bits[i] == 1)
            {
              d_tx_buffer.insert(d_tx_buffer.end(), data_1.begin(), data_1.end());
            }
            else
            {
              d_tx_buffer.insert(d_tx_buffer.end(), data_0.begin(), data_0.end());
            }
          }
          // Send CW for RN16
          d_tx_buffer.insert(d_tx_buffer.end(), cw_query.begin(), cw_query.end());

          // Return to IDLE
          reader_state->gen2_logic_status = IDLE;      
          break;

        case SEND_ACK:
          if (ninput_items[0] == RN16_BITS - 1)
          {
            // Controls the other two blocks
            reader_state->decoder_status = DECODER_DECODE_EPC;
            reader_state->gate_status    = GATE_SEEK_EPC;

            gen_ack_bits(in);
          
            // Send FrameSync
            d_tx_buffer.insert(d_tx_buffer.end(), frame_sync.begin(), frame_sync.end());

            for(int i = 0; i < ack_bits.size(); i++)
            {
              if(ack_bits[i] == 1)
              {
                d_tx_buffer.insert(d_tx_buffer.end(), data_1.begin(), data_1.end());
              }
              else  
              {
                d_tx_buffer.insert(d_tx_buffer.end(), data_0.begin(), data_0.end());
              }
            }
            consumed = ninput_items[0];
            reader_state->gen2_logic_status = SEND_CW; 
          }
          break;

        case SEND_CW:
          // 替换成你的自定义 OFDM 波形：
          d_tx_buffer.insert(d_tx_buffer.end(), AugCW.begin(), AugCW.end());
          reader_state->gen2_logic_status = IDLE;      // Return to IDLE
          break;

        case SEND_QUERY_REP:
          reader_state->decoder_status = DECODER_DECODE_RN16;
          reader_state->gate_status    = GATE_SEEK_RN16;
          reader_state->reader_stats.n_queries_sent +=1;  

          d_tx_buffer.insert(d_tx_buffer.end(), query_rep.begin(), query_rep.end());
          d_tx_buffer.insert(d_tx_buffer.end(), cw_query.begin(), cw_query.end());

          reader_state->gen2_logic_status = IDLE;    // Return to IDLE
          break;
      
        case SEND_QUERY_ADJUST:
          reader_state->decoder_status = DECODER_DECODE_RN16;
          reader_state->gate_status    = GATE_SEEK_RN16;
          reader_state->reader_stats.n_queries_sent +=1;  

          d_tx_buffer.insert(d_tx_buffer.end(), frame_sync.begin(), frame_sync.end());

          for(int i = 0; i < query_adjust_bits.size(); i++)
          {
            if(query_adjust_bits[i] == 1)
            {
              d_tx_buffer.insert(d_tx_buffer.end(), data_1.begin(), data_1.end());
            }
            else
            {
              d_tx_buffer.insert(d_tx_buffer.end(), data_0.begin(), data_0.end());
            }
          }
          d_tx_buffer.insert(d_tx_buffer.end(), cw_query.begin(), cw_query.end());
          reader_state->gen2_logic_status = IDLE;    // Return to IDLE
          break;

        default:
          // IDLE
          break;
      }

      // ==========================================
      // 阶段 4：如果本轮生成了新数据，立刻发送第一块
      // ==========================================
      int items_to_send = 0;
      if (!d_tx_buffer.empty()) {
          items_to_send = std::min((size_t)noutput_items, d_tx_buffer.size());
          
          // [修改] sizeof 改为 gr_complex
          memcpy(out, &d_tx_buffer[0], items_to_send * sizeof(gr_complex));
          
          d_tx_index += items_to_send;
      }

      consume_each (consumed);
      return items_to_send;
    }

    /* Function adapted from https://www.cgran.org/wiki/Gen2 */
    void reader_impl::crc_append(std::vector<float> & q)
    {
       int crc[] = {1,0,0,1,0};

      for(int i = 0; i < 17; i++)
      {
        int tmp[] = {0,0,0,0,0};
        tmp[4] = crc[3];
        if(crc[4] == 1)
        {
          if (q[i] == 1)
          {
            tmp[0] = 0;
            tmp[1] = crc[0];
            tmp[2] = crc[1];
            tmp[3] = crc[2];
          }
          else
          {
            tmp[0] = 1;
            tmp[1] = crc[0];
            tmp[2] = crc[1];
            if(crc[2] == 1)
            {
              tmp[3] = 0;
            }
            else
            {
              tmp[3] = 1;
            }
          }
        }
        else
        {
          if (q[i] == 1)
          {
            tmp[0] = 1;
            tmp[1] = crc[0];
            tmp[2] = crc[1];
            if(crc[2] == 1)
            {
              tmp[3] = 0;
            }
            else
            {
              tmp[3] = 1;
            }
          }
          else
          {
            tmp[0] = 0;
            tmp[1] = crc[0];
            tmp[2] = crc[1];
            tmp[3] = crc[2];
          }
        }
        memcpy(crc, tmp, 5*sizeof(float));
      }
      for (int i = 4; i >= 0; i--)
        q.push_back(crc[i]);
    }
  } /* namespace rfid */
} /* namespace gr */