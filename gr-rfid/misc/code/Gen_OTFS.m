clc;
clear;
close all;

%% 参数设置
M = 5;                       % 延迟维度（子载波数）
N = 5;                       % 多普勒维度（符号数）
Delta_f = 500e3;              % 子载波间隔 (125 kHz)
B = N * Delta_f;              % 总带宽 2 MHz
T = 1 / (M * Delta_f);        % 符号时长 (0.5 μs)
cp_length = 5;                % 循环前缀长度
QAM_order = 4;                % QAM 调制阶数（例如 4 表示 QPSK）
disp(['OTFS 总带宽: ', num2str(B / 1e6), ' MHz']);
disp(['OTFS 符号时长: ', num2str(T * 1e6), ' μs']);

%% 发送端

% 1. 比特流生成与符号映射 (QPSK)
data_bits = randi([0 QAM_order-1], M, N);               % 随机生成数据符号
symbols = qammod(data_bits, QAM_order, 'UnitAveragePower', true); % QAM 调制
symbols_vector = symbols(:); % 将二维矩阵展平成一维向量
% figure,
% scatterplot(symbols_vector);
% figure,
% mesh(abs(symbols));
% title('Delay-Doppler');
% 2. ISFFT：延迟-多普勒域  -> 时频域
% 逐列进行 IFFT（延迟维度）
time_freq_grid = zeros(M, N);
for l = 1:N
    time_freq_grid(:, l) = ifft(symbols(:, l),N); % 对每一列进行 IFFT
end
% figure,
% mesh(abs(time_freq_grid));
% title('Time-Doppler');

% 逐行进行 FFT（多普勒维度）
for m = 1:M
    time_freq_grid(m, :) = fft(time_freq_grid(m, :),N); % 对每一行进行 FFT
end

% 3. OFDM 调制：对时频域信号进行 IFFT
ofdm_signal = zeros(M, N);
for m = 1:M
    ofdm_signal(m, :) = ifft(time_freq_grid(m, :),N); % 对每一行进行 IFFT，生成 OFDM 时域信号
end

% 4. 添加循环前缀
ofdm_with_cp = [ofdm_signal(:, end-cp_length+1:end), ofdm_signal]; % 添加 CP
tx_signal = reshape(ofdm_with_cp.', [], 1); % 展平成一维时域信号


OTFS_cw = [];
for i = 1:length(tx_signal)
    OTFS_cw = [OTFS_cw, 'gr_complex(', num2str(real(tx_signal(i))), ', ', num2str(imag(tx_signal(i))), '), '];
end
OTFS_cw(end-1:end) = [];  % 删除最后的逗号

% 绘制发送信号
figure;
plot(abs(tx_signal));
title('OTFS 发送信号（时域）');
xlabel('样本点');
ylabel('幅度');
% 
% %% 信道模型
% 
% % 模拟多径信道（简化模型）
% h_channel = [1, 0.5, 0.3];              % 多径增益
% delay = [0, 2, 4];                      % 多径时延（样本点）
% doppler_shift = [0, 1, -1];             % 多普勒频移
% 
% rx_signal = zeros(size(tx_signal));
% 
% for i = 1:length(h_channel)
%     % 多径影响：时延和多普勒频移
%     shifted_signal = circshift(tx_signal, delay(i)); % 添加时延
%     doppler_factor = exp(1j * 2 * pi * doppler_shift(i) * (0:length(tx_signal)-1)' / length(tx_signal));
%     rx_signal = rx_signal + h_channel(i) * shifted_signal .* doppler_factor;
% end
% 
% % 添加高斯白噪声
% rx_signal = awgn(rx_signal, 20, 'measured'); % 信噪比 20 dB
% 
% % 绘制接收信号
% figure;
% plot(abs(rx_signal));
% title('OTFS 接收信号（时域）');
% xlabel('样本点');
% ylabel('幅度');
% 
% %% 接收端
% 
% % 1. 去掉循环前缀
% rx_signal_matrix = reshape(rx_signal, N + cp_length, []).'; % 每行一个 OFDM 符号（含 CP）
% rx_signal_no_cp = rx_signal_matrix(:, cp_length+1:end);      % 去掉 CP
% 
% % 2. OFDM 解调：对每一行进行 FFT
% rx_time_freq_grid = zeros(M, N);
% for m = 1:M
%     rx_time_freq_grid(m, :) = fft(rx_signal_no_cp(m, :)); % 对每一行进行 FFT，恢复时频域信号
% end
% 
% % 3. SFFT：时频域 -> 延迟-多普勒域
% % 逐列进行 FFT（多普勒维度）
% rx_delay_doppler_grid = zeros(M, N);
% for m = 1:M
%     rx_delay_doppler_grid(m, :) = ifft(rx_time_freq_grid(m, :)); % 对每一行进行 IFFT（多普勒维度）
% end
% 
% % 逐行进行 FFT（延迟维度）
% for l = 1:N
%     rx_delay_doppler_grid(:, l) = fft(rx_delay_doppler_grid(:, l)); % 对每一列进行 FFT（延迟维度）
% end
% 
% % 4. 符号检测 (QPSK 判决)
% detected_symbols = qamdemod(rx_delay_doppler_grid, QAM_order, 'UnitAveragePower', true);
% detected_bits = detected_symbols(:);
% 
% % 5. 性能评估
% original_bits = data_bits(:);
% bit_errors = sum(detected_bits ~= original_bits);
% ber = bit_errors / length(original_bits);
% disp(['误符号数：', num2str(bit_errors)]);
% disp(['误符号率 (BER): ', num2str(ber)]);
% 
% % 绘制接收信号的延迟-多普勒图
% figure;
% imagesc(abs(rx_delay_doppler_grid));
% title('接收信号的延迟-多普勒图');
% xlabel('多普勒索引');
% ylabel('延迟索引');
% colorbar;
