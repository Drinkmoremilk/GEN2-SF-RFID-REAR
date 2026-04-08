#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as plt
import os

# ==========================================
# 参数设置区
# ==========================================
num_points = 200000  # <--- 你可以在这里任意修改你想绘制的点数

# 清除之前的图形
plt.close('all')

# 获取脚本所在目录，构建数据文件路径
script_dir = os.path.dirname(os.path.abspath(__file__))
source_file_path = os.path.normpath(os.path.join(script_dir, '../data/source'))
# 假设你的 sink 文件名为 'sink'，如果不是，请修改下面这行
sink_file_path = os.path.normpath(os.path.join(script_dir, '../data/file_sink')) 

# ==========================================
# 核心读取函数（防止代码重复，提高效率）
# ==========================================
def read_complex_data(file_path, max_points):
    if not os.path.exists(file_path):
        print(f"⚠️ 警告: 找不到文件 {file_path}")
        return np.array([])
    
    # 巧妙利用 count 参数，只读取我们需要的数据量，防止大文件撑爆内存。
    # 一个复数由实部和虚部 2 个 float32 组成，所以要乘以 2
    with open(file_path, 'rb') as f:
        data_inter = np.fromfile(f, dtype=np.float32, count=max_points * 2)
    
    # 转换为复数格式
    complex_data = data_inter[0::2] + 1j * data_inter[1::2]
    return complex_data

# 读取数据
print(f"正在读取前 {num_points} 个点...")
source_data = read_complex_data(source_file_path, num_points)
sink_data = read_complex_data(sink_file_path, num_points)

# ==========================================
# 绘图区
# ==========================================
plt.figure(figsize=(12, 6)) # 画布拉宽一点，方便看清 20000 个点的波形细节

# 绘制 Source (蓝色，实线，带一点透明度)
if len(source_data) > 0:
    plt.plot(np.abs(source_data), label='Source (RX)', color='blue', alpha=0.7)

# 绘制 Sink (红色，虚线，带一点透明度，防止完全遮挡 Source)
if len(sink_data) > 0:
    plt.plot(np.abs(sink_data), label='Sink (TX)', color='red', alpha=0.7, linestyle='--')

# 图表修饰
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.title(f'Source vs Sink Amplitude Comparison (First {num_points} Points)')
plt.grid(True, linestyle=':', alpha=0.6)
plt.legend(loc='upper right') # 显示右上角的图例

# 自动紧凑布局并显示

plt.tight_layout()
plt.show()