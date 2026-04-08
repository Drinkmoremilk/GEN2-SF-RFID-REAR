#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
import os
import random

# ==========================================
# 1. 核心参数配置
# ==========================================
EPC_LENGTH = 91500 
MIN_GAP_BETWEEN_ROUNDS = 120000 
READ_SAMPLES = 170000000 

# 【核心新增】：前置跳过区 (上电稳定时间)
# 20 MHz 采样率下，1_000_000 点 = 50 ms，足够 USRP 和 PA 完全进入稳态
SEARCH_START = 1000000 

script_dir = os.path.dirname(os.path.abspath(__file__))
rx_file_path = os.path.normpath(os.path.join(script_dir, '../data/source'))
ref_file_path = os.path.normpath(os.path.join(script_dir, '/home/yelab/Desktop/jyc/Gen2-UHF-RFID-Reader_AugCW_adaptive_ADC/sweep_paprMix_3.83dB_sar0dB.txt')) 

# ==========================================
# 2. 读取并拼装“完整版”参考波形
# ==========================================
print("正在构建 91500 点完整参考波形...")
ref_base = []
with open(ref_file_path, 'r') as f:
    for line in f:
        parts = line.strip().split()
        if len(parts) == 2:
            ref_base.append(complex(float(parts[0]), float(parts[1])))
ref_base = np.array(ref_base, dtype=np.complex64)

repeat_count = EPC_LENGTH // len(ref_base)
remainder = EPC_LENGTH % len(ref_base)

ref_full = np.concatenate([
    np.tile(ref_base, repeat_count),
    ref_base[:remainder]
]).astype(np.complex64)

# ==========================================
# 3. 读取接收端全局 Raw 数据
# ==========================================
print(f"正在读取接收数据 (前 {READ_SAMPLES} 个点)...")
with open(rx_file_path, 'rb') as f:
    rx_inter = np.fromfile(f, dtype=np.float32, count=READ_SAMPLES * 2)

# rx_data_all 是包含上电不稳定期的全局数据
rx_data_all = rx_inter[0::2] + 1j * rx_inter[1::2]

# ==========================================
# 4. 【核心新增】：截断上电期，准备搜索数据
# ==========================================
if len(rx_data_all) <= SEARCH_START:
    raise ValueError("读取的数据太短了，连上电跳过区都不够！请增加 READ_SAMPLES。")

# rx_data 是剥离了上电不稳定期后的“纯净搜索区”
rx_data = rx_data_all[SEARCH_START:]

# ==========================================
# 5. 去均值与归一化 (仅在搜索区进行)
# ==========================================
ref_use = ref_full - np.mean(ref_full)
rx_use = rx_data - np.mean(rx_data)

ref_use = ref_use / (np.linalg.norm(ref_use) + 1e-12)

# ==========================================
# 6. FFT 全长互相关与精确寻峰
# ==========================================
print("正在执行 FFT 匹配滤波 (互相关)...")
corr = signal.correlate(rx_use, ref_use, mode='valid', method='fft')
corr_abs = np.abs(corr)

print("正在定位精确对齐的稳态峰值...")
threshold = 0.5 * np.max(corr_abs)
# 直接用自带的 distance 替代手写的冷却逻辑，既优雅又快速
peaks, _ = signal.find_peaks(corr_abs, height=threshold, distance=MIN_GAP_BETWEEN_ROUNDS)

print(f"✅ 在稳定区成功找到 {len(peaks)} 个有效起点！")

# ==========================================
# 7. 【新增】：精细化裁剪参数计算 (EPC Gen2 物理层映射)
# ==========================================
SAMPLE_RATE = 20e6
T1_US = 240               # T1 等待时间 (微秒)
TAG_BIT_US = 25           # Tag 单个符号时间 (微秒)
TAG_PREAMBLE_BITS = 6     # 标签前导码位数
EPC_DATA_BITS = 129       # EPC 数据总位数 (根据你的实际设定调整)

# 换算为采样点
T1_SAMPLES = int(T1_US * 1e-6 * SAMPLE_RATE)
TAG_BIT_SAMPLES = int(TAG_BIT_US * 1e-6 * SAMPLE_RATE)
EPC_TOTAL_BITS = TAG_PREAMBLE_BITS + EPC_DATA_BITS
EPC_SAMPLES = EPC_TOTAL_BITS * TAG_BIT_SAMPLES

# 计算偏移量与截取长度
# 起点偏移 = T1时间 - 2个Tag_bit的裕度
OFFSET_START = T1_SAMPLES - 2 * TAG_BIT_SAMPLES
# 提取总长 = 2个Tag_bit裕度 + 完整EPC长度
EXTRACT_LENGTH = 2 * TAG_BIT_SAMPLES + EPC_SAMPLES+2* TAG_BIT_SAMPLES

print(f"\n✂️ 精细化裁剪参数:")
print(f" -> 相对 AugCW 起点后移: {OFFSET_START} 个点")
print(f" -> 实际提取数据段长度: {EXTRACT_LENGTH} 个点")

# ==========================================
# 8. 批量切片，提取纯净 EPC 矩阵
# ==========================================
valid_starts = []
for p in peaks:
    real_s = p + SEARCH_START 
    # 确保加上偏移量和提取长度后，依然不会越界
    if real_s + OFFSET_START + EXTRACT_LENGTH <= len(rx_data_all):
        valid_starts.append(real_s)

valid_starts = np.array(valid_starts, dtype=int)
num_to_extract = min(1000, len(valid_starts))

# 矩阵的列数现在变成了我们精细计算的 EXTRACT_LENGTH
epc_matrix = np.zeros((num_to_extract, EXTRACT_LENGTH), dtype=np.complex64)

for i in range(num_to_extract):
    s = valid_starts[i]
    # 【核心】：从偏移量开始切，切到提取长度结束
    epc_matrix[i, :] = rx_data_all[s + OFFSET_START : s + OFFSET_START + EXTRACT_LENGTH]

print(f"🎉 最终切片完成！精细化矩阵 epc_matrix 形状: {epc_matrix.shape}")

# ==========================================
# 9. 绘图验证 (高亮标注随机抽查版)
# ==========================================

num_to_plot = min(3, num_to_extract)
check_indices = random.sample(range(num_to_extract), num_to_plot)
check_indices.sort() 

plt.figure(figsize=(15, 8))
plt.suptitle(f'Targeted Extraction: EPC Segments #{check_indices[0]+1}, #{check_indices[1]+1}, #{check_indices[2]+1}', 
             fontsize=16, fontweight='bold')

for i, idx in enumerate(check_indices):
    plt.subplot(num_to_plot, 1, i + 1)
    plt.plot(np.abs(epc_matrix[idx]), color='#2ca02c') # 换个护眼的绿色
    
    # 画一条辅助线，标出 Tag 真正开始回复的理论物理位置 (也就是前 2 个 bit 结束的地方)
    plt.axvline(x=2 * TAG_BIT_SAMPLES, color='black', linestyle='--', label='Tag Backscatter Start')
    
    plt.title(f'Segment #{idx+1} (Absolute Amplitude)', fontsize=12)
    plt.ylabel('Amplitude')
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.xlim(0, EXTRACT_LENGTH)
    if i == 0:
        plt.legend(loc='upper right')

plt.xlabel('Sample Index (0 = T1 end - 2 bits)')
plt.tight_layout()
plt.subplots_adjust(top=0.88)
plt.show()