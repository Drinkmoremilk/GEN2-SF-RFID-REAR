#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import os

# ==========================================
# 1. 基础 OFDM 波形生成 (不含 CW)
# ==========================================
N = 150             # 子载波数
win = np.concatenate(([0.5], np.ones(N-2), [0.5]))

np.random.seed(3937)  # 固定种子
TS_f = np.sign(np.random.rand(N) * 2 - 1)
TS_f[0] = 0         
TS_f[N//2] = 0      
TS_f[-1] = 0        

TS_f_shifted = np.fft.ifftshift(TS_f)
TS_t_base = np.fft.ifft(TS_f_shifted) * win

P_ofdm_base = np.mean(np.abs(TS_t_base)**2)
P_cw = 1.0**2 # 基底 CW 功率

# ==========================================
# 2. 批量生成不同功率占比 (AugCW / CW) 的波形
# ==========================================
# 修正后的测试列表：负数代表 AugCW 弱，正数代表 AugCW 强
sar_db_list = [-30, -20, -15, -10, -5, 0, 5] 

print(f"{'目标 SAR(dB)':<15} | {'缩放因子 alpha':<15} | {'归一化系数':<15} | {'文件名'}")
print("-" * 65)

for sar_db in sar_db_list:
    # 修正公式：P_AugCW = P_CW * 10^(SAR_dB / 10)
    sar_linear = 10 ** (sar_db / 10.0)
    P_ofdm_target = P_cw * sar_linear
    
    # 计算 alpha (因为 P_ofdm_target = alpha^2 * P_ofdm_base)
    alpha = np.sqrt(P_ofdm_target / P_ofdm_base)
    
    # 叠加原始信号: 1.0 + alpha * OFDM
    x_raw = 1.0 + alpha * TS_t_base
    
    # ------------------------------------------
    # 全局归一化，防止 USRP 硬件削峰
    # ------------------------------------------
    max_amp = np.max([np.abs(x_raw.real), np.abs(x_raw.imag)])
    
    norm_factor = 1.0
    if max_amp > 0.95:
        norm_factor = 0.95 / max_amp  
        x_norm = x_raw * norm_factor
    else:
        x_norm = x_raw
        
    filename = f"augcw_sar_{sar_db}dB.txt"
    with open(filename, "w") as f:
        for val in x_norm:
            f.write(f"{val.real:.4f} {val.imag:.4f}\n")
            
    print(f"{sar_db:<15} | {alpha:<15.4f} | {norm_factor:<15.4f} | {filename}")

print("-" * 65)
print("✅ 所有波形文件已基于正确的 AugCW/CW 占比生成！")