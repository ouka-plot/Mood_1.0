# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an STM32F407-based audio player project that plays WAV files from an SD card with real-time display on a TFT LCD. The project uses FreeRTOS for task management and includes LVGL for GUI capabilities.

**Hardware Platform**: STM32F407ZGTx (Cortex-M4, 168MHz, 1MB Flash, 192KB RAM including 64KB CCM)

**Key Features**:

- WAV audio playback from SD card (0:/MUSIC directory)
- ES8388 audio codec via I2S interface
- LVGL-based GUI with multi-language font support (Chinese, Japanese, Korean)
- Audio monitoring for high-frequency sound detection (e.g., baby crying)
- Key-based controls (next/previous/pause/play/restart)
- FreeRTOS-based multitasking architecture

## Build System

**Primary Toolchain**: GCC + Docker

**Build Commands**:

- Build: run `make` in the project root, use `tools/build/build.sh`, or use the VS Code `Build (Docker)` task
- Clean build: run `make clean`, `tools/build/build.sh clean`, or use the VS Code `Clean Build (Docker)` task
- Flash: use `tools/flash/flash.bat`, `tools/flash/jlink_flash.jlink`, or the VS Code `Flash (J-Link)` task
- Output files are generated in `Output/` directory

**Note**: Legacy IDE project files are no longer kept in the repository. The maintained build flow is GCC/Docker-based.

## Architecture

### Directory Structure

```text
├── Drivers/
│   ├── CMSIS/              # ARM CMSIS core and STM32F4 device headers
│   ├── STM32F4xx_HAL_Driver/  # STM32 HAL library
│   ├── BSP/                # Board Support Package drivers
│   │   ├── ES8388/         # Audio codec driver
│   │   ├── I2S/            # I2S audio interface
│   │   ├── LCD/            # TFT LCD driver
│   │   ├── LED/            # LED control
│   │   ├── KEY/            # Key input handling
│   │   ├── SDIO/           # SD card interface
│   │   └── IIC/            # I2C communication
│   └── SYSTEM/             # System utilities
│       ├── sys/            # System initialization
│       ├── delay/          # Delay functions
│       └── usart/          # UART communication
├── FreeRTOS/               # FreeRTOS kernel source
│   ├── tasks.c, queue.c, timers.c, etc.
│   └── portable/           # FreeRTOS heap implementations (heap_1 to heap_5)
├── Middlewares/
│   ├── FATFS/              # FAT filesystem for SD card
│   ├── AUDIOCODEC/wav/     # WAV file decoder
│   ├── LVGL/               # LittlevGL graphics library
│   └── USMART/             # Debug command interface
├── User/
│   ├── main.c              # Application entry point
│   ├── FreeRTOSConfig.h    # FreeRTOS configuration
│   ├── stm32f4xx_hal_conf.h  # HAL configuration
│   ├── stm32f4xx_it.c      # Interrupt handlers
│   └── APP/
│       ├── audioplay.c     # Audio playback logic
│       └── audio_ui_bridge.c/h  # Bridge between audio task and UI task
├── UI/generated/           # LVGL GUI generated files
│   ├── gui_guider.c/h      # GUI setup and structure
│   ├── events_init.c       # Event handlers
│   └── guider_fonts/       # Custom fonts (CJK support)
├── tools/                  # Build and flash helper scripts
│   ├── build/build.sh      # Docker build helper
│   └── flash/              # J-Link flashing helpers
└── Output/                 # Build output directory
```


### FreeRTOS Task Architecture

The application uses FreeRTOS V9.0.0 with multiple tasks:

1. **vAudio_Task** (Priority 2, Stack: 1024 words)
   - Runs `audio_play()` which manages the audio playback loop
   - Scans SD card for WAV files in `0:/MUSIC/`
   - Handles track navigation and playback control
   - Updates shared state via `audio_ui_bridge` for UI display
   - Blocks on I2S DMA transfers during playback

2. **vLvgl_Task** (Priority 2, Stack: configurable)
   - Runs LVGL event loop (`lv_timer_handler()`)
   - Reads shared state from `audio_ui_bridge` and updates GUI widgets
   - Handles button events and visual feedback
   - Updates every 10ms for smooth UI rendering

3. **vLed_Task** (Priority 2, Stack: configMINIMAL_STACK_SIZE)
   - Toggles LED1 every 1000ms as a heartbeat indicator
   - Simple status indicator task

**Task Communication**:

- Audio task and UI task communicate via `audio_ui_bridge.c` shared state structure
- No dynamic memory allocation; all buffers are statically allocated

**Memory Configuration**:

- Uses heap_5 with CCM RAM region: 0x10000000 (64KB)
- Priority group: NVIC_PRIORITYGROUP_4 (4 bits for preemption priority)
- Tick rate: 1000Hz (configTICK_RATE_HZ)

### Audio Playback Flow

1. **Initialization** (main.c):
   - HAL, clocks (168MHz), peripherals init
   - SD card mount to `0:/MUSIC`
   - ES8388 codec configuration (DAC enabled, ADC disabled)
   - FreeRTOS tasks creation and scheduler start

2. **Audio Task Loop** (audioplay.c):
   - Scan `0:/MUSIC/` directory for WAV files (max 200 files)
   - Build offset table for directory navigation
   - For each track:
     - Display filename and track index on LCD
     - Call `wav_play_song()` which handles WAV decoding and I2S DMA
     - Process key inputs (KEY0=next, KEY2=prev, KEY_WAKE=restart)
   - Loop continues until error or user exit

3. **WAV Playback** (Middlewares/AUDIOCODEC/wav/):
   - Parse WAV header (RIFF, fmt, data chunks)
   - Configure I2S based on sample rate and bit depth
   - DMA-based audio streaming (buffer size: 8192 bytes)
   - Supports 16-bit and 24-bit WAV files up to 192kHz

### Key Input Handling

- **KEY0**: Next track
- **KEY2**: Previous track
- **KEY1**: Pause/Resume (if implemented)
- **KEY_WAKE**: Restart current track from beginning

### Memory Management

**Important**: This project was modified to remove dynamic memory allocation (malloc/free) to avoid heap fragmentation issues. Audio playback uses static buffers:

- `s_wav_offset_tbl[200]`: Static array for file offsets
- `s_pname_buf[FF_MAX_LFN * 2 + 1]`: Static buffer for file paths
- `MAX_MUSIC_FILES`: Hard limit of 200 audio files

## Common Development Tasks

### Adding New BSP Drivers

BSP drivers follow a consistent pattern:

- Header in `Drivers/BSP/<PERIPHERAL>/<peripheral>.h`
- Implementation in `Drivers/BSP/<PERIPHERAL>/<peripheral>.c`
- Include initialization function called from main.c
- Use STM32 HAL for hardware abstraction

### Modifying Audio Playback

Key files:

- `User/APP/audioplay.c`: High-level playback control and UI
- `User/APP/audio_ui_bridge.c/h`: Shared state between audio and UI tasks
- `Middlewares/AUDIOCODEC/wav/wavplay.c`: WAV decoder and I2S streaming
- `Drivers/BSP/I2S/i2s.c`: I2S peripheral and DMA configuration
- `Drivers/BSP/ES8388/es8388.c`: Audio codec control via I2C

### FreeRTOS Configuration

Edit `User/FreeRTOSConfig.h` for:

- Heap size, tick rate, task priorities
- Enable/disable features (mutexes, semaphores, timers, etc.)
- Stack overflow detection, malloc failed hooks

### Adding FreeRTOS Tasks

1. Define task function: `static void vMyTask(void *pvParameters)`
2. Create task in main.c before `vTaskStartScheduler()`:

   ```c
   xTaskCreate(vMyTask, "TaskName", stackSize, NULL, priority, &taskHandle);
   ```

3. Ensure total stack usage fits in available RAM (128KB SRAM + 64KB CCM)

### Debugging

- USART1 configured at 115200 baud for printf debugging
- USMART debug interface available for runtime command execution
- J-Link or ST-Link can be used for on-target flashing/debugging
- FreeRTOS hooks for malloc failures and stack overflows in main.c

## LVGL Configuration

The project includes LVGL v8.3.9 for GUI with a custom music player interface (screen_1).

**UI Architecture**:

- GUI files generated in `UI/generated/` directory
- `gui_guider.h/c`: Main GUI structure and screen setup functions
- `events_init.c`: Button event handlers
- `audio_ui_bridge.c`: Decouples audio task from UI task via shared state

**Custom Fonts**:

- `lv_font_song_20`: 20px font with CJK (Chinese, Japanese, Korean) character support
- Includes common artist names, song titles, and UI text in multiple languages
- Located in `UI/generated/guider_fonts/`

**Screen_1 Widgets** (Music Player UI):

- `screen_1_label_1`: Current time display (e.g., "1:23")
- `screen_1_label_2`: Total time display (e.g., "3:45")
- `screen_1_label_3`: Song name with CJK font support
- `screen_1_bar_1`: Progress bar (0-100%)
- `screen_1_btn_1`: Previous track button
- `screen_1_btn_2`: Next track button
- `screen_1_btn_3`: Play/Pause button (toggles LV_SYMBOL_PLAY/PAUSE)
- `screen_1_img_1`: Album art or static image (shown when paused)
- `screen_1_chart_1`: Audio visualization chart (shown when playing)

**UI Update Pattern**:

- Audio task writes to `g_audio_ui` shared state structure
- UI task calls `audio_ui_update_lvgl()` every 10ms to read state and update widgets
- Button press visual feedback handled with timer-based state management

**Enabled Features**:

- Basic widgets: Label, Button, Bar, Image, Chart
- Single input device: Keypad (hardware keys mapped to LVGL keys)
- Fonts: Montserrat 14 (default), Montserrat 16, lv_font_song_20 (CJK)
- Memory: 24KB LVGL heap
- Color depth: RGB565 (16-bit)
- Theme: Default light theme (no animations to save memory)

**Disabled Features** (to save Flash/RAM):

- Extra widgets (calendar, keyboard, list, menu, etc.)
- GPU acceleration
- Image decoders (PNG, JPG, GIF)
- Logging and debug monitors
- Multiple input devices (touchpad, mouse, encoder, button)

**Key Mapping** (in `lv_port_indev.c`):

- KEY0 → LV_KEY_RIGHT (Next)
- KEY1 → LV_KEY_ENTER (Confirm)
- KEY2 → LV_KEY_LEFT (Previous)
- KEY_WAKE → LV_KEY_ESC (Back/Cancel)

**Display Porting** (`lv_port_disp.c`):

- Uses existing LCD driver from `Drivers/BSP/LCD/`
- Single buffer mode (10 rows)
- Flush callback writes directly to LCD

## Important Notes

- **Build System**: Uses GCC Makefile plus Docker build helpers; run `make` in project root, or use `tools/build/build.sh` and the VS Code build tasks
- **Static Memory**: Dynamic allocation removed; use static buffers for new features
- **SD Card Path**: Audio files must be in `0:/MUSIC/` directory
- **File Limit**: Maximum 200 audio files supported (MAX_MUSIC_FILES)
- **Audio Format**: Only WAV files supported (16/24-bit PCM, up to 192kHz)
- **Chinese Comments**: Many comments are in Chinese; code structure is standard C
- **Interrupt Priorities**: FreeRTOS uses priority 5-15; keep critical interrupts at 0-4
- **CCM RAM**: 64KB CCM at 0x10000000 used for FreeRTOS heap (not DMA-accessible)
- **LVGL Memory**: 24KB heap for LVGL (configured in lv_conf.h)
- **I2S Full-Duplex**: Audio monitoring requires full-duplex I2S (TX + RX simultaneously)
- **DMA Buffers**: Audio monitor uses double buffering for continuous recording without data loss
- **CJK Font**: Custom font file is large (~several MB); regenerate with LVGL font converter if needed

## File Encoding

Source files contain Chinese characters in comments. Ensure your editor uses GB2312 or UTF-8 encoding to display them correctly.
