clc;
clear all;
close all;

T = 100e-6;                % Pulse width: 100 microseconds (note: originally commented as 25 microseconds)
B = 0.2e6;                 % Bandwidth: 100 kHz (note: originally commented as 500 kHz)
k = B/T;                   % Chirp rate (frequency sweep slope)
alpha = 1;
fs = 2e6;
N = fs*T;
t = linspace(0, T, N);
f = linspace(0, fs, N);
chirp_signal = exp(1i*k*pi*t.^2)*alpha;   % Linear Frequency Modulated (LFM) signal
figure;
plot(t, chirp_signal);                  
title('Linear Frequency Modulated (LFM) Signal');
xlabel('Time (s)');
ylabel('Amplitude');  % Single pulse

figure;
plot(f, abs(fft(chirp_signal)));                  
title('LFM Signal Spectrum');
xlabel('Frequency (Hz)');
ylabel('Amplitude');  % Spectrum of single pulse

% Convert the chirp signal to gr_complex format (similar to original code)
chirp_cw = [];
for i = 1:length(chirp_signal)
    chirp_cw = [chirp_cw, 'gr_complex(', num2str(real(chirp_signal(i))), ', ', num2str(imag(chirp_signal(i))), '), '];
end
chirp_cw(end-1:end) = [];  % Remove the trailing comma

fid = fopen('output1.txt', 'w');  % Open file for writing
fprintf(fid, '%s', chirp_cw);     % Write the data to file
fclose(fid);                      % Close the file
