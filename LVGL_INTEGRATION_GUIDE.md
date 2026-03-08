# LVGL适配快速指南 - STM32F407 + 320×240触摸屏

## 📌 你的项目配置

| 项目 | 配置 |
|------|------|
| **MCU** | STM32F407ZGTx (168MHz, 192KB SRAM, 1MB Flash) |
| **屏幕** | 2.8寸电容触摸屏 |
| **接口** | FSMC 8080 |
| **分辨率** | 320×240 (RGB565) |
| **LVGL版本** | 8.3.9+ |
| **实时OS** | FreeRTOS |

---

## ✅ 第一步：启用LVGL配置文件（lv_conf.h）

### 1.1 打开 `Middlewares/LVGL/GUI/lvgl/lv_conf.h`

**第15行** - 启用配置：
```c
// 改这一行：
#if 0 /*Set it to "1" to enable content*/

// 改成：
#if 1 /*Set it to "1" to enable content*/
```

### 1.2 配置内存和显示参数

搜索并修改以下参数：

```c
// 第47-48行：内存配置（STM32F407需要节省）
#define LV_MEM_SIZE (32U * 1024U)        // 改成32KB
#define LV_MEM_BUF_MAX_NUM 8              // 改成8

// 显示分辨率（你的屏幕大小）
#define LV_HOR_RES_MAX 320               // 宽度
#define LV_VER_RES_MAX 240               // 高度
#define LV_COLOR_DEPTH 16                // RGB565格式（已是默认）

// 输入设备配置
#define LV_INDEV_DEF_READ_PERIOD 30      // 30ms读取周期
#define LV_DISP_DEF_REFR_PERIOD 30       // 30ms刷新周期
```

---

## ✅ 第二步：启用Port驱动文件

你的项目在 `Middlewares/LVGL/GUI/lvgl/porting/` 目录已有模板文件：

### 2.1 启用显示驱动 (`lv_port_disp.c` / `.h`)

打开 `Middlewares/LVGL/GUI/lvgl/porting/lv_port_disp.h`

**第7行** - 启用内容：
```c
/*Copy this file as "lv_port_disp.h" and set this value to "1" to enable content*/
#if 0              // ❌ 改成

/*Copy this file as "lv_port_disp.h" and set this value to "1" to enable content*/
#if 1              // ✅ 启用
```

打开 `Middlewares/LVGL/GUI/lvgl/porting/lv_port_disp.c`

**第7行** - 同样启用：
```c
/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 0              // ❌ 改成

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1              // ✅ 启用
```

### 2.2 修改 `lv_port_disp.c` 中的LCD操作

在 `lv_port_disp.c` 中找到 `disp_flush` 函数，修改为调用你项目的LCD驱动函数：

```c
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    // 这里需要你实现LCD写数据的逻辑
    // 根据你的lcd.c中的函数进行调用，例如：
    
    uint16_t x1 = area->x1, y1 = area->y1;
    uint16_t x2 = area->x2, y2 = area->y2;
    uint16_t width = x2 - x1 + 1;
    uint16_t height = y2 - y1 + 1;
    
    // 设置显示窗口（调用你的LCD函数）
    lcd_set_window(x1, y1, width, height);
    
    // 准备写入GRAM
    lcd_write_ram_prepare();
    
    // 写入像素数据
    uint32_t size = width * height;
    uint16_t *buf = (uint16_t *)color_p;
    for(uint32_t i = 0; i < size; i++) {
        lcd_wr_data(buf[i]);
    }
    
    lv_disp_flush_ready(disp_drv);
}
```

### 2.3 启用输入设备驱动（按键操作）

打开 `Middlewares/LVGL/GUI/lvgl/porting/lv_port_indev.h` 和 `.c`

**第7行** - 同样改为 `#if 1` 启用

编辑 `lv_port_indev.c` 中的 `indev_read_cb` 函数，适配你的按键：

```c
static void indev_read_cb(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    // 读取按键 (你的key_scan函数)
    uint8_t key = key_scan(0);
    
    // 根据按键映射到LVGL的导航
    if (key == KEY0_PRES) {
        data->key = LV_KEY_RIGHT;      // 下一个
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (key == KEY2_PRES) {
        data->key = LV_KEY_LEFT;       // 上一个
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (key == KEY1_PRES) {
        data->key = LV_KEY_ENTER;      // 确认
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (key == KEY_UP_PRES) {
        data->key = LV_KEY_ESC;        // 返回
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
```

### 2.4 其他Port文件

- `lv_port_fs.h / .c` - 文件系统：用FAT FS，可暂不启用
- `lv_port_indev.h / .c` - 已在2.3中说明

---

## ✅ 第三步：修改main.c集成LVGL

### 3.1 添加LVGL头文件

在 `User/main.c` 顶部添加：
```c
#include "lvgl.h"
#include "lv_port_disp.h"       // 显示驱动
#include "lv_port_indev.h"      // 输入设备

// 移除或注释TEXT库
// #include "./TEXT/text.h"
```

### 3.2 初始化LVGL

在 `main()` 函数中，LCD初始化之后添加：

```c
void main(void)
{
    HAL_Init();
    // ... 其他初始化 ...
    
    lcd_init();                 // LCD初始化
    
    /* NEW: LVGL初始化 */
    lv_init();                  // 初始化LVGL核心
    lv_port_disp_init();        // 初始化显示驱动
    lv_port_indev_init();       // 初始化输入设备
    
    // 创建UI（可选）
    // create_ui();
    
    // 启动FreeRTOS
    vTaskStartScheduler();
}
```

### 3.3 添加FreeRTOS Tick Hook（自动更新LVGL时钟）

修改 `User/FreeRTOSConfig.h`：
```c
#define configUSE_TICK_HOOK 1  // 启用tick hook
```

在 `User/stm32f4xx_it.c` 或其他适当位置添加（或在 `main.c` 中）：

```c
/* FreeRTOS Tick Hook - LVGL需要调用此函数来更新内部时钟 */
void vApplicationTickHook(void)
{
    lv_tick_inc(1);  // 每1ms增加一次LVGL计时器
}
```

### 3.4 创建LVGL UI任务（推荐）

创建一个任务定期刷新LVGL UI：

```c
static void vLVGL_Task(void *pvParameters)
{
    for (;;) {
        lv_timer_handler();  // 处理LVGL内部计时器事件
        vTaskDelay(pdMS_TO_TICKS(5));  // 每5ms执行一次
    }
}

// 在main()中创建任务：
xTaskCreate(vLVGL_Task, "LVGL", 512, NULL, 1, NULL);
```

---

## ✅ 第四步：Keil项目配置

### 4.1 检查Project源文件

**Project → Manage Project Items**

确保以下文件在项目中（通常需要手动确认路径）：

| 文件 | 路径 | 状态 |
|------|------|------|
| lv_port_disp.c | `Middlewares/LVGL/GUI/lvgl/porting/` | ✅ 需要 |
| lv_port_indev.c | `Middlewares/LVGL/GUI/lvgl/porting/` | ✅ 需要 |
| lv_port_fs.c | `Middlewares/LVGL/GUI/lvgl/porting/` | ⓘ 可选 |

**删除的源文件**：
```
❌ Middlewares/TEXT/text.c
❌ Middlewares/TEXT/fonts.c
```

### 4.2 检查包含路径

**Project → Options for Target → C/C++ → Include Paths**

```
.\Middlewares\LVGL\GUI\lvgl\src
.\Middlewares\LVGL\GUI\lvgl\examples
.\Middlewares\LVGL\GUI\lvgl\porting
```

---

## ✅ 第五步：编译和测试

### 5.1 编译

```
Project → Build Target (Ctrl+F7)
```

预期无链接错误。如果有错误：

| 错误 | 解决方案 |
|------|---------|
| "undefined reference to lcd_wr_data" | 检查lcd.c是否已添加到项目 |
| "lv_conf.h not found" | 检查包含路径是否正确 |
| "multiple definition" | 检查是否有多个lv_conf.h被包含 |

### 5.2 下载和测试

```
Flash → Download
```

复位开发板，应该看到：
- ✅ LCD背光亮起
- ✅ LVGL显示内容
- ✅ 按键可控制LVGL焦点

---

## 📊 内存使用预估

```
STM32F407 总内存 (256KB)

SRAM分配:
├── TEXT库版本 (旧):   10KB
│   ├── 显示缓冲: 10KB
│   └── 其他: 0KB
│
├── LVGL版本 (新):     45KB
│   ├── LVGL内存池: 32KB (lv_conf.h配置)
│   ├── 显示缓冲: 12KB (320x40 RGB565)
│   └── 其他: 1KB
│
└── FreeRTOS: 50KB (不变)
```

✅ **足够** - 剩余~150KB给音频等功能

---

## 🎨 简单UI示例

创建基本的音乐播放器UI：

```c
void create_music_ui(void)
{
    lv_obj_t *scr = lv_scr_act();
    
    // 清空屏幕
    lv_obj_clean(scr);
    
    // 标题
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Music Player");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // 歌曲信息
    lv_obj_t *song_name = lv_label_create(scr);
    lv_label_set_text(song_name, "Song: Unknown");
    lv_obj_align(song_name, LV_ALIGN_CENTER, 0, -30);
    
    // 播放按钮
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 80, 40);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Play");
    lv_obj_center(btn_label);
    
    // 进度条
    lv_obj_t *bar = lv_bar_create(scr);
    lv_obj_set_size(bar, 200, 10);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_bar_set_value(bar, 50, LV_ANIM_OFF);
}

// 在main()中调用: create_music_ui();
```

---

## ❌ 可以删除的文件

### 必删：TEXT库

```
❌ Middlewares/TEXT/text.c
❌ Middlewares/TEXT/text.h  
❌ Middlewares/TEXT/fonts.c  （如果未被其他地方使用）
❌ Middlewares/TEXT/fonts.h  （如果未被其他地方使用）
```

**第一步**：在main.c中替换所有 `text_show_string()` 为LVGL `lv_label_create()`

**第二步**：从Keil项目中移除text.c和fonts.c

**第三步**：删除物理文件或保留备份

### 可选删：示例代码（节省200KB Flash）

```
ⓘ Middlewares/LVGL/GUI_APP/demos/     （演示程序）
ⓘ Middlewares/LVGL/examples/          （示例代码）
```

---

## 🔧 常见问题

### Q: 屏幕显示花屏？
**A:** 
- 检查 `lv_port_disp.c` 中的 `disp_flush()` 函数是否正确调用了LCD驱动
- 检查 `lv_conf.h` 的分辨率（320x240）是否正确
- 减小缓冲区高度（改为 `320x20` 而不是 `320x40`）

### Q: 按键无反应？
**A:**
- 确保 `lv_port_indev.c` 中的 `indev_read_cb()` 正确调用了 `key_scan()`
- 检查FreeRTOS Tick Hook是否启用
- 检查KEY驱动是否初始化

### Q: 编译超出Flash限制？
**A:**
- 删除 `Middlewares/LVGL/GUI_APP/demos/` 目录（节省200KB）
- 删除TEXT库源文件
- 优化LVGL配置（禁用不需要的widgets）

### Q: RAM不足？
**A:**
- 减小 `LV_MEM_SIZE` 到 `24KB`
- 减小显示缓冲高度（从40改为20行）

---

## 📚 完整检查清单

- [ ] lv_conf.h 改为 `#if 1`
- [ ] 配置分辨率 (320x240)，内存大小 (32KB)
- [ ] lv_port_disp.c 和 .h 改为 `#if 1`
- [ ] lv_port_indev.c 和 .h 改为 `#if 1`
- [ ] 修改 `disp_flush()` 调用项目的LCD函数
- [ ] 修改 `indev_read_cb()` 调用项目的key_scan()
- [ ] main.c 添加包含头文件
- [ ] main.c 添加初始化代码
- [ ] FreeRTOSConfig.h 启用 `configUSE_TICK_HOOK`
- [ ] Keil项目添加port文件
- [ ] 删除或注释TEXT库调用
- [ ] 编译测试
- [ ] 下载运行验证

---

## 🚀 立即开始

1. 打开 `Middlewares/LVGL/GUI/lvgl/lv_conf.h`，改第15行的 `#if 0` → `#if 1`
2. 配置分辨率和内存
3. 启用port文件（改为 `#if 1`）
4. 修改port文件以适配你的LCD和KEY驱动
5. 在main.c中添加初始化代码
6. 编译测试

有任何问题请参考本指南的对应章节！
