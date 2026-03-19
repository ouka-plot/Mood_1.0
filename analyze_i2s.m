%% I2S / PCM 逻辑分析仪数据分析 (修正版)
% 正确解析 0x... 十六进制数据，提取低 16 位有符号 PCM

clear; clc; close all;

%% 1. 读取 CSV — 强制 data 和 channel 列为字符串
opts = detectImportOptions('logic.csv', 'VariableNamingRule', 'preserve');
dataIdx = strcmp(opts.VariableNames, 'data');
opts.VariableTypes(dataIdx) = {'string'};
chIdx = strcmp(opts.VariableNames, 'channel');
opts.VariableTypes(chIdx) = {'string'};

T = readtable('logic.csv', opts);

% 过滤掉 channel 或 data 为空/missing 的行
valid = ~ismissing(T.channel) & ~ismissing(T.data);
T = T(valid, :);
fprintf('有效数据行: %d (过滤 %d 行)\n', sum(valid), sum(~valid));

start_time = T.start_time;  % 时间戳 (s)

% 解析 channel："0x0000000000000000" -> 0,  "0x0000000000000001" -> 1
ch_hex = extractAfter(T.channel, '0x');
channel = uint8(hex2dec(ch_hex));

% 解析 data：提取低 16 位，typecast 为 int16
data_hex = extractAfter(T.data, '0x');
low16_hex = extractAfter(data_hex, strlength(data_hex) - 4);  % 最后4位hex
raw_u16 = uint16(hex2dec(low16_hex));
samples = typecast(raw_u16, 'int16');  % 有符号 int16

%% 2. 分离左右声道
idx_L = (channel == 0);
idx_R = (channel == 1);

time_L = start_time(idx_L);
time_R = start_time(idx_R);
pcm_L  = double(samples(idx_L));
pcm_R  = double(samples(idx_R));

%% 3. 估算采样率
dt = mean(diff(time_L));
Fs = round(1 / dt);
fprintf('估算采样率: %d Hz\n', Fs);
fprintf('左声道样本数: %d\n', length(pcm_L));
fprintf('右声道样本数: %d\n', length(pcm_R));
fprintf('总时长: %.4f s\n', time_L(end) - time_L(1));

%% 4. 时域波形
figure('Name', 'I2S 时域波形', 'Position', [100 100 1200 600]);

subplot(2,1,1);
plot(time_L * 1000, pcm_L, 'b-', 'LineWidth', 0.5);
xlabel('时间 (ms)');
ylabel('幅值');
title('左声道 (Channel 0)');
ylim([-32768 32767]);
grid on;

subplot(2,1,2);
plot(time_R * 1000, pcm_R, 'r-', 'LineWidth', 0.5);
xlabel('时间 (ms)');
ylabel('幅值');
title('右声道 (Channel 1)');
ylim([-32768 32767]);
grid on;

sgtitle('I2S PCM 时域波形');

%% 5. 频谱分析
N_L = length(pcm_L);
N_R = length(pcm_R);

win_L = hanning(N_L);
win_R = hanning(N_R);

Y_L = fft(pcm_L .* win_L);
Y_R = fft(pcm_R .* win_R);

f_L = (0:N_L-1) * (Fs / N_L);
f_R = (0:N_R-1) * (Fs / N_R);

mag_L = 20 * log10(abs(Y_L(1:floor(N_L/2)+1)) / N_L + eps);
mag_R = 20 * log10(abs(Y_R(1:floor(N_R/2)+1)) / N_R + eps);

f_L_half = f_L(1:floor(N_L/2)+1);
f_R_half = f_R(1:floor(N_R/2)+1);

figure('Name', 'I2S 频谱', 'Position', [100 100 1200 600]);

subplot(2,1,1);
plot(f_L_half / 1000, mag_L, 'b-', 'LineWidth', 0.5);
xlabel('频率 (kHz)');
ylabel('幅值 (dB)');
title('左声道频谱');
xlim([0 Fs/2000]);
grid on;

subplot(2,1,2);
plot(f_R_half / 1000, mag_R, 'r-', 'LineWidth', 0.5);
xlabel('频率 (kHz)');
ylabel('幅值 (dB)');
title('右声道频谱');
xlim([0 Fs/2000]);
grid on;

sgtitle('I2S PCM 频谱分析');

%% 6. 左右声道对比（前 50ms 局部放大）
figure('Name', '左右声道对比', 'Position', [100 100 1200 500]);

subplot(2,1,1);
plot(time_L * 1000, pcm_L, 'b-', 'LineWidth', 0.5); hold on;
plot(time_R * 1000, pcm_R, 'r-', 'LineWidth', 0.5);
xlabel('时间 (ms)');
ylabel('幅值');
title('左右声道对比 (全局)');
ylim([-32768 32767]);
legend('左声道 (CH0)', '右声道 (CH1)');
grid on;

subplot(2,1,2);
t_zoom = 50;  % 放大前 50ms
idx_zoom_L = time_L * 1000 <= t_zoom;
idx_zoom_R = time_R * 1000 <= t_zoom;
plot(time_L(idx_zoom_L) * 1000, pcm_L(idx_zoom_L), 'b-', 'LineWidth', 0.8); hold on;
plot(time_R(idx_zoom_R) * 1000, pcm_R(idx_zoom_R), 'r-', 'LineWidth', 0.8);
xlabel('时间 (ms)');
ylabel('幅值');
title(sprintf('左右声道对比 (前 %d ms 放大)', t_zoom));
legend('左声道 (CH0)', '右声道 (CH1)');
grid on;

%% 7. 基本统计
fprintf('\n--- 统计信息 ---\n');
fprintf('左声道: 最大=%d, 最小=%d, RMS=%.1f\n', max(pcm_L), min(pcm_L), rms(pcm_L));
fprintf('右声道: 最大=%d, 最小=%d, RMS=%.1f\n', max(pcm_R), min(pcm_R), rms(pcm_R));

dbfs_L = 20 * log10(max(abs(pcm_L)) / 32767);
dbfs_R = 20 * log10(max(abs(pcm_R)) / 32767);
fprintf('左声道峰值: %.1f dBFS\n', dbfs_L);
fprintf('右声道峰值: %.1f dBFS\n', dbfs_R);

%% 8. 验证检查清单
fprintf('\n===== 验证检查 =====\n');

% 检查 1: 数值范围是否正确（应有正有负）
has_neg_L = any(pcm_L < 0);
has_neg_R = any(pcm_R < 0);
fprintf('[%s] 左声道含负值: min=%d\n', iff(has_neg_L,'OK','FAIL'), min(pcm_L));
fprintf('[%s] 右声道含负值: min=%d\n', iff(has_neg_R,'OK','FAIL'), min(pcm_R));

% 检查 2: 直流偏移检查（均值应接近 0）
dc_L = mean(pcm_L);
dc_R = mean(pcm_R);
fprintf('[%s] 左声道直流偏移: %.1f (应接近0)\n', iff(abs(dc_L)<500,'OK','WARN'), dc_L);
fprintf('[%s] 右声道直流偏移: %.1f (应接近0)\n', iff(abs(dc_R)<500,'OK','WARN'), dc_R);

% 检查 3: 采样率是否在预期范围 (44.1k / 48k / 96k)
expected_rates = [44100, 48000, 96000];
rate_diff = min(abs(Fs - expected_rates));
fprintf('[%s] 采样率 %d Hz (最近标准值差: %d Hz)\n', iff(rate_diff<200,'OK','WARN'), Fs, rate_diff);

% 检查 4: 左右声道样本数差异
ch_diff = abs(length(pcm_L) - length(pcm_R));
fprintf('[%s] 左右声道样本差: %d\n', iff(ch_diff<10,'OK','WARN'), ch_diff);

% 检查 5: 是否削波 (超过 1% 样本达满量程)
clip_L = sum(abs(pcm_L) >= 32767) / length(pcm_L) * 100;
clip_R = sum(abs(pcm_R) >= 32767) / length(pcm_R) * 100;
fprintf('[%s] 左声道削波率: %.2f%%\n', iff(clip_L<1,'OK','WARN'), clip_L);
fprintf('[%s] 右声道削波率: %.2f%%\n', iff(clip_R<1,'OK','WARN'), clip_R);

%% 辅助函数
function s = iff(cond, yes, no)
    if cond, s = yes; else, s = no; end
end
