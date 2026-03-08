# LVGL Optimization Summary

This document describes the LVGL optimizations made to reduce memory footprint for the STM32F407 music player project.

## Files Modified

1. **Middlewares/LVGL/GUI/lvgl/porting/lv_port_indev.c**
   - Removed: Touchpad, Mouse, Encoder, Button input devices
   - Kept: Keypad only (mapped to hardware keys)
   - Reduced from ~420 lines to ~80 lines
   - Memory saved: ~2KB Flash

2. **Middlewares/LVGL/GUI/lvgl/lv_conf.h**
   - Optimized all configuration settings
   - Disabled unnecessary widgets and features
   - See details below

## Memory Optimization

### LVGL Heap
- **Before**: 32KB
- **After**: 24KB
- **Saved**: 8KB RAM

### Layer Buffers
- **Before**: 24KB simple buffer, 3KB fallback
- **After**: 12KB simple buffer, 2KB fallback
- **Saved**: 13KB RAM

### Circle Cache
- **Before**: 4 cached circles
- **After**: 2 cached circles
- **Saved**: ~1KB RAM

### Total RAM Savings: ~22KB

## Widgets Enabled (Minimal Set)

Only the essential widgets for a music player UI:

- **LV_USE_LABEL**: Text display (song name, time, etc.)
- **LV_USE_BTN**: Control buttons (play, pause, next, prev)
- **LV_USE_BAR**: Progress bar for playback position
- **LV_USE_SLIDER**: Volume control
- **LV_USE_IMG**: Icons and album art

## Widgets Disabled

All non-essential widgets disabled to save Flash:

- Arc, Button Matrix, Canvas, Checkbox, Dropdown
- Line, Roller, Switch, Textarea, Table
- Calendar, Chart, Color Wheel, Image Button
- Keyboard, LED, List, Menu, Meter, Message Box
- Span, Spinbox, Spinner, Tab View, Tile View, Window

**Estimated Flash Savings**: ~50-80KB

## Features Disabled

### GPU Acceleration
All GPU backends disabled (not available on STM32F407):
- ARM-2D, STM32 DMA2D, RA6M3 G2D, SWM341 DMA2D
- NXP PXP, NXP VG-Lite, SDL

### Image Decoders
- PNG, BMP, SJPG, GIF, QR Code
- Use built-in LVGL image format only

### Advanced Features
- Logging (LV_USE_LOG = 0)
- Performance monitor
- Memory monitor
- Bidirectional text (BIDI)
- Arabic/Persian text processing
- Font compression
- Subpixel rendering
- Snapshot, Monkey test, Grid navigation
- Fragment, IME Pinyin

### Themes
- **Enabled**: Default theme (light mode, no grow animation)
- **Disabled**: Basic theme, Mono theme

### Layouts
- **Enabled**: Flex layout
- **Disabled**: Grid layout

## Fonts Enabled

Minimal font set:
- **Montserrat 14**: Default font
- **Montserrat 16**: Larger text (optional)

All other Montserrat sizes (8, 10, 12, 18-48) disabled.

**Flash Savings**: ~100KB

## Input Device Configuration

Hardware key mapping in `lv_port_indev.c`:

```c
KEY0_PRES      → LV_KEY_RIGHT   // Next track
KEY1_PRES      → LV_KEY_ENTER   // Confirm/Play/Pause
KEY2_PRES      → LV_KEY_LEFT    // Previous track
KEY_WAKE_PRES  → LV_KEY_ESC     // Back/Cancel
```

## Build Impact

### Before Optimization
- LVGL Flash usage: ~250KB (estimated)
- LVGL RAM usage: ~60KB (estimated)

### After Optimization
- LVGL Flash usage: ~120KB (estimated)
- LVGL RAM usage: ~38KB (estimated)

### Total Savings
- **Flash**: ~130KB
- **RAM**: ~22KB

## Usage Guidelines

### Adding New UI Elements

When creating UI, only use enabled widgets:

```c
// ✓ Allowed
lv_obj_t *label = lv_label_create(parent);
lv_obj_t *btn = lv_btn_create(parent);
lv_obj_t *bar = lv_bar_create(parent);
lv_obj_t *slider = lv_slider_create(parent);
lv_obj_t *img = lv_img_create(parent);

// ✗ Not available (will cause linker errors)
lv_obj_t *chart = lv_chart_create(parent);      // Disabled
lv_obj_t *calendar = lv_calendar_create(parent); // Disabled
lv_obj_t *keyboard = lv_keyboard_create(parent); // Disabled
```

### Re-enabling Features

If you need a disabled feature:

1. Edit `Middlewares/LVGL/GUI/lvgl/lv_conf.h`
2. Change the corresponding `#define` from `0` to `1`
3. Rebuild the project
4. Monitor Flash/RAM usage to ensure it fits

### Example: Enable Chart Widget

```c
// In lv_conf.h, change:
#define LV_USE_CHART 0
// To:
#define LV_USE_CHART 1
```

## Performance Notes

- Display refresh: 30ms (33 FPS)
- Input polling: 30ms
- No animations enabled (GROW disabled)
- Transition time: 80ms

These settings balance responsiveness with CPU usage for the 168MHz STM32F407.

## Recommendations

1. **Keep it minimal**: Only enable features you actually use
2. **Test memory usage**: Use `make size` to check Flash/RAM after changes
3. **Profile performance**: If UI feels sluggish, increase refresh period
4. **Use static allocation**: Avoid creating/destroying objects frequently
5. **Optimize images**: Use LVGL's built-in image converter for smaller size

## Further Optimization Possibilities

If you need more memory:

1. Reduce LVGL heap to 16KB (if UI is simple)
2. Disable Flex layout if not using it
3. Use only Montserrat 14 font (disable 16)
4. Reduce display buffer from 10 rows to 5 rows
5. Disable label text selection
6. Set LV_MEM_BUF_MAX_NUM to 4 (from 8)

Each of these can save additional 2-8KB of RAM.
