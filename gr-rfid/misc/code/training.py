import numpy as np
import matplotlib.pyplot as plt
from scipy.io import loadmat

# 定义参数
nepc = 128
N = 50

# 读取 EPC 文件
def read_fec_file(filename):
    with open(filename, 'rb') as fid:
        data = np.fromfile(fid, dtype=np.float32)
    return data

# 读取 EPC 数据
x_inter_1 = read_fec_file('../data/hezi/epc0')
x_inter_2 = read_fec_file('../data/hezi/epc1')
x_inter_3 = read_fec_file('../data/hezi/epc2')
x_inter_4 = read_fec_file('../data/hezi/epc3')

# 将实部和虚部组合成复数
x_1 = x_inter_1[::2] + 1j * x_inter_1[1::2]
x_2 = x_inter_2[::2] + 1j * x_inter_2[1::2]
x_3 = x_inter_3[::2] + 1j * x_inter_3[1::2]
x_4 = x_inter_4[::2] + 1j * x_inter_4[1::2]

# 加载 TS 数据
TS = loadmat("TS_t.mat")
TS_t = TS['TS_t'].flatten()

# 计算 TS 的 FFT 并绘图
TS_f = np.fft.fftshift(np.fft.fft(TS_t))
plt.figure()
plt.plot(np.abs(TS_f))
plt.title('FFT of TS_t')
plt.show()

# 提取 EPC 数据
TS_t_epc1 = x_1[:50 * nepc]
TS_t_epc2 = x_2[:50 * nepc]
TS_t_epc3 = x_3[:50 * nepc]
TS_t_epc4 = x_4[:50 * nepc]

# 绘制 EPC 数据的时间域波形
plt.figure()
plt.subplot(4, 1, 1)
plt.plot(np.abs(TS_t_epc1))
plt.title('TS_t_epc1')
plt.subplot(4, 1, 2)
plt.plot(np.abs(TS_t_epc2))
plt.title('TS_t_epc2')
plt.subplot(4, 1, 3)
plt.plot(np.abs(TS_t_epc3))
plt.title('TS_t_epc3')
plt.subplot(4, 1, 4)
plt.plot(np.abs(TS_t_epc4))
plt.title('TS_t_epc4')
plt.tight_layout()
plt.show()

# 计算每个 EPC 的 FFT 平均结果
TS_f_epc1 = np.zeros(N, dtype=np.complex128)
TS_f_epc2 = np.zeros(N, dtype=np.complex128)
TS_f_epc3 = np.zeros(N, dtype=np.complex128)
TS_f_epc4 = np.zeros(N, dtype=np.complex128)

for i in range(nepc):
    chunk1 = TS_t_epc1[i * N:(i + 1) * N] - np.mean(TS_t_epc1[i * N:(i + 1) * N])
    chunk2 = TS_t_epc2[i * N:(i + 1) * N] - np.mean(TS_t_epc2[i * N:(i + 1) * N])
    chunk3 = TS_t_epc3[i * N:(i + 1) * N] - np.mean(TS_t_epc3[i * N:(i + 1) * N])
    chunk4 = TS_t_epc4[i * N:(i + 1) * N] - np.mean(TS_t_epc4[i * N:(i + 1) * N])

    TS_f_epc1 += np.fft.fftshift(np.fft.fft(chunk1))
    TS_f_epc2 += np.fft.fftshift(np.fft.fft(chunk2))
    TS_f_epc3 += np.fft.fftshift(np.fft.fft(chunk3))
    TS_f_epc4 += np.fft.fftshift(np.fft.fft(chunk4))

TS_f_epc1 /= nepc
TS_f_epc2 /= nepc
TS_f_epc3 /= nepc
TS_f_epc4 /= nepc

# 计算共轭并绘制对数散射图
hx_1 = np.abs(TS_f_epc1) / np.abs(TS_f)
hx_2 = np.abs(TS_f_epc2) / np.abs(TS_f)
hx_3 = np.abs(TS_f_epc3) / np.abs(TS_f)
hx_4 = np.abs(TS_f_epc4) / np.abs(TS_f)

plt.figure()
plt.plot(np.log10(np.abs(hx_1)), label='hx_1')
plt.plot(np.log10(np.abs(hx_2)), label='hx_2')
plt.plot(np.log10(np.abs(hx_3)), label='hx_3')
plt.plot(np.log10(np.abs(hx_4)), label='hx_4')
plt.legend()
plt.title('Logarithmic Scattering Ratio')
plt.grid(True)
plt.show()
