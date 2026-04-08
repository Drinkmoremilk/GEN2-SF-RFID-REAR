#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import numpy as np

# ==========================================
# 1. 基础参数与 PAPR_mix 蒙特卡洛搜索
# ==========================================
N = 150
win = np.concatenate(([0.5], np.ones(N-2), [0.5]))
num_search = 5000  
results = []

P_cw_theory = 0.5
P_aug_theory = 0.5

print(f"正在搜索 {num_search} 个随机种子，按【叠加后波形 (PAPR_mix)】评估...")

for seed in range(num_search):
    np.random.seed(seed)
    TS_f = np.sign(np.random.rand(N) * 2 - 1)
    TS_f[0] = 0; TS_f[N//2] = 0; TS_f[-1] = 0
    
    TS_f_shifted = np.fft.ifftshift(TS_f)
    TS_t = np.fft.ifft(TS_f_shifted) * win
    
    # 【修复 1】：强制去均值，抵消窗函数带来的直流泄漏
    TS_t = TS_t - np.mean(TS_t)
    
    # 归一化基础 OFDM 到平均功率为 1.0
    P_base = np.mean(np.abs(TS_t)**2)
    s_norm = TS_t / np.sqrt(P_base)
    
    # 立即生成混合波形 x_raw
    x_raw = np.sqrt(P_cw_theory) + np.sqrt(P_aug_theory) * s_norm
    
    # 【修复 2】：计算混合后波形的 PAPR (PAPR_mix)
    power_peak_mix = np.max(np.abs(x_raw)**2)
    # 因为强制去均值了，这里的 power_avg_mix 将严格等于 1.0
    power_avg_mix = np.mean(np.abs(x_raw)**2) 
    papr_mix_db = 10 * np.log10(power_peak_mix / power_avg_mix)
    
    results.append((papr_mix_db, seed, x_raw))

# 按混合波形的 PAPR 从小到大排序
results.sort(key=lambda x: x[0])

# ==========================================
# 2. 均匀抽取 6 个代表性波形 (从最平坦 到 最陡峭)
# ==========================================
num_waveforms = 6
step = len(results) // (num_waveforms - 1)
selected_indices = [i * step for i in range(num_waveforms - 1)] + [len(results) - 1]

selected_waveforms = [results[i] for i in selected_indices]

# ==========================================
# 3. 寻找全局最高峰，计算唯一的全局缩放因子
# ==========================================
global_max_peak = 0.0
for papr_mix_db, seed, x_raw in selected_waveforms:
    local_max = np.max([np.abs(x_raw.real), np.abs(x_raw.imag)])
    if local_max > global_max_peak:
        global_max_peak = local_max

# 留出 0.05 的安全裕度，防止 USRP 削峰
global_scale = 0.95 / global_max_peak if global_max_peak > 0.95 else 1.0

# ==========================================
# 4. 【修复 3】：全局缩放并验证严格的数学恒等
# ==========================================
print("\n✅ 提取完毕！固定 SAR = 0 dB (CW 50%, AugCW 50%)")
print(f"{'Seed':<6} | {'混合波形 PAPR':<15} | {'最终总功率 (严格恒定)':<22} | {'最终最大峰值':<12} | {'文件名'}")
print("-" * 90)

for papr_mix_db, seed, x_raw in selected_waveforms:
    # 使用同一个比例尺进行缩放
    x_final = x_raw * global_scale
    
    # 重新核算最终总功率
    P_total_final = np.mean(np.abs(x_final)**2)
    max_peak_final = np.max([np.abs(x_final.real), np.abs(x_final.imag)])
    
    filename = f"sweep_paprMix_{papr_mix_db:.2f}dB_sar0dB.txt"
    with open(filename, "w") as f:
        for val in x_final:
            f.write(f"{val.real:.4f} {val.imag:.4f}\n")
            
    print(f"{seed:<6} | {papr_mix_db:<12.2f} dB | {P_total_final:<22.10f} | {max_peak_final:<12.4f} | {filename}")

print("-" * 90)
print(f"全局缩放因子: {global_scale:.6f}")
print("所有弹药已就绪，最终总功率已做到小数点后 10 位严格一致！祝实验顺利！")