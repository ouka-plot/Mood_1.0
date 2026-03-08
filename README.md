# Mood_1.0

基于 STM32F407 和 FreeRTOS 的 WAV 音频播放与实时音频处理项目。

## 功能概览

- 从 SD 卡 `0:/MUSIC` 目录读取并播放 WAV 音频
- 使用 ES8388 + I2S 进行音频编解码与输出
- 使用 LVGL 图形界面显示歌曲名、时间、进度和 EQ 页面
- 支持按键控制上一曲、下一曲、暂停/播放和重新播放
- 支持音频监听、频谱分析、降噪、特征提取和 TinyML 分类任务
- 支持串口 EQ 控制，并可通过 nRF52832 BLE bridge 进行无线下发

## 目录结构

- `User/APP`：应用层逻辑，包括播放器、EQ、监听、TinyML、UI bridge
- `Drivers`：STM32 HAL、BSP 和系统底层驱动
- `Middlewares`：FATFS、WAV 解码、LVGL、USMART 等中间件
- `UI`：LVGL 生成界面与自定义界面代码
- `tools`：构建与烧录辅助脚本
- `nrf52832_ble_bridge`：Zephyr/NCS BLE-UART 桥接子工程
- `Output`：编译输出目录

## 构建

当前维护的主构建链是 GCC + Docker。

### 方式 1：VS Code 任务

- 使用 `Build (Docker)` 任务编译
- 使用 `Flash (J-Link)` 任务烧录

### 方式 2：命令行

```bash
make
```

或使用 Docker 构建脚本：

```bash
./tools/build/build.sh
```

清理并重建：

```bash
./tools/build/build.sh clean
```

## 烧录

可使用以下任一方式烧录：

- `tools/flash/flash.bat`
- `tools/flash/jlink_flash.jlink`
- VS Code 的 `Flash (J-Link)` 任务

## 说明

- 项目已移除旧的 TEXT 字库显示模块，当前界面字体统一由 LVGL 字体资源提供
- 编译输出位于 `Output/` 目录
- 音频文件格式当前以 WAV 为主
- BLE 桥接子工程说明见 `nrf52832_ble_bridge/README.md`
