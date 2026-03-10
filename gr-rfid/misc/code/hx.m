clc
clear all
close all
nepc = 128;
N=50;
fi_1 = fopen('../data/hezi/epc0','rb');
x_inter_1 = fread(fi_1, 'float32');
x_1 = x_inter_1(1:2:end) + 1i*x_inter_1(2:2:end);
% figure,plot(abs(x_1))
fi_2 = fopen('../data/hezi/epc1','rb');
x_inter_2 = fread(fi_2, 'float32');
x_2 = x_inter_2(1:2:end) + 1i*x_inter_2(2:2:end);
% figure,plot(abs(x_2))
fi_3 = fopen('../data/hezi/epc2','rb');
x_inter_3 = fread(fi_3, 'float32');
x_3 = x_inter_3(1:2:end) + 1i*x_inter_3(2:2:end);
% figure,plot(abs(x_2))
fi_4 = fopen('../data/hezi/epc3','rb');
x_inter_4 = fread(fi_4, 'float32');
x_4 = x_inter_4(1:2:end) + 1i*x_inter_4(2:2:end);
% figure,plot(abs(x_3))
TS = load("TS_t.mat");
TS_t = TS. TS_t;
% figure,plot(abs(TS_t));
TS_f = fftshift(fft((TS_t)))';
% TS_f(50/2+1)=1;
figure,plot(abs(TS_f));
% TS_t_epc1 = x_1(1:50);
% TS_t_epc2 = x_2(1:50);
% TS_t_epc3 = x_3(1:50);
TS_t_epc1 = x_1(1:50*128);
TS_t_epc2 = x_2(1:50*128);
TS_t_epc3 = x_3(1:50*128);
TS_t_epc4 = x_4(1:50*128);
figure,
subplot(411);plot(abs(TS_t_epc1));
subplot(412);plot(abs(TS_t_epc2));
subplot(413);plot(abs(TS_t_epc3));
subplot(414);plot(abs(TS_t_epc4));
TS_f_epc1 = zeros(1,50)';
TS_f_epc2 = zeros(1,50)';
TS_f_epc3 = zeros(1,50)';
TS_f_epc4 = zeros(1,50)';
for i = 1:nepc
    TS_f_epc1 = TS_f_epc1 + fftshift(fft(TS_t_epc1((i-1)*50+1:i*50)-mean(TS_t_epc1((i-1)*50+1:i*50))));
    TS_f_epc2 = TS_f_epc2 + fftshift(fft(TS_t_epc2((i-1)*50+1:i*50)-mean(TS_t_epc2((i-1)*50+1:i*50))));
    TS_f_epc3= TS_f_epc3 + fftshift(fft(TS_t_epc3((i-1)*50+1:i*50)-mean(TS_t_epc3((i-1)*50+1:i*50))));
    TS_f_epc4= TS_f_epc4 + fftshift(fft(TS_t_epc4((i-1)*50+1:i*50)-mean(TS_t_epc4((i-1)*50+1:i*50))));
end
TS_f_epc1 = TS_f_epc1/nepc;
TS_f_epc2 = TS_f_epc2/nepc;
TS_f_epc3 = TS_f_epc3/nepc;
TS_f_epc4 = TS_f_epc4/nepc;
% TS_f_epc1 = fft(fftshift(TS_t_epc1-mean(TS_t_epc1)));
% TS_f_epc2 = fft(fftshift(TS_t_epc2-mean(TS_t_epc2)));
% TS_f_epc3 = fft(fftshift(TS_t_epc3-mean(TS_t_epc3)));
% figure,
% subplot(311);plot(abs(TS_f_epc1));
% subplot(312);plot(abs(TS_f_epc2));
% subplot(313);plot(abs(TS_f_epc3));

hx_1 = TS_f_epc1./TS_f;
hx_2 = TS_f_epc2./TS_f;
hx_3 = TS_f_epc3./TS_f;
hx_4 = TS_f_epc4./TS_f;

figure,
plot(log10(abs(hx_1)));
hold on
plot(log10(abs(hx_2)));
hold on 
plot(log10(abs(hx_3)));
hold on
plot(log10(abs(hx_4)));