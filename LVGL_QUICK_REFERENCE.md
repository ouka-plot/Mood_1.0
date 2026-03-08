# LVGL移植快速参考卡

## 🎯 你的项目配置
- **MCU**: STM32F407ZGTx (168MHz, 192KB SRAM, 1MB Flash)
- **LCD**: 2.8寸电容屏, FSMC 8080驱动
- **分辨率**: 320×240 RGB565
- **LVGL版本**: 8.3.9
- **Port文件位置**: `Middlewares/LVGL/GUI/lvgl/porting/`

---

## ⚡ 5分钟快速启动

### 步骤1：启用lv_conf.h (1分钟)
```c
// 文件: Middlewares/LVGL/GUI/lvgl/lv_conf.h
// 第15行改:
#if 0  →  #if 1

// 然后搜索并改这些参数:
#define LV_HOR_RES_MAX 320          // 宽度
#define LV_VER_RES_MAX 240          // 高度
#define LV_MEM_SIZE (32U * 1024U)   // 32KB
#define LV_MEM_BUF_MAX_NUM 8        // 8个缓冲
#define LV_COLOR_DEPTH 16           // RGB565
```

### 步骤2：启用Port文件 (1分钟)
```c
// 文件1: Middlewares/LVGL/GUI/lvgl/porting/lv_port_disp.h
// 第7行改: #if 0  →  #if 1

// 文件2: Middlewares/LVGL/GUI/lvgl/porting/lv_port_disp.c
// 第7行改: #if 0  →  #if 1

// 文件3: Middlewares/LVGL/GUI/lvgl/porting/lv_port_indev.h & .c
// 同样改: #if 0  →  #if 1
```

### 步骤3：适配LCD驱动 (2分钟)
```c
// 文件: Middlewares/LVGL/GUI/lvgl/porting/lv_port_disp.c
// 找到 disp_flush() 函数，修改为:

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    uint16_t x1 = area->x1, y1 = area->y1;
    uint16_t x2 = area->x2, y2 = area->y2;
    
    lcd_set_window(x1, y1, x2-x1+1, y2-y1+1);  // 你的LCD函数
    lcd_write_ram_prepare();                      // 你的LCD函数
    
    uint32_t size = (x2-x1+1) * (y2-y1+1);
    uint16_t *buf = (uint16_t *)color_p;
    for(uint32_t i = 0; i < size; i++) {
        lcd_wr_data(buf[i]);                      // 你的LCD函数
    }
    
    lv_disp_flush_ready(disp_drv);
}
```

### 步骤4：适配按键驱动 (1分钟)
```c
// 文件: Middlewares/LVGL/GUI/lvgl/porting/lv_port_indev.c
// 找到 indev_read_cb() 函数，修改关键部分:

uint8_t key = key_scan(0);  // 你的按键函数

if (key == KEY0_PRES) {
    data->key = LV_KEY_RIGHT;
    data->state = LV_INDEV_STATE_PRESSED;
} 
// ... 更多按键映射
```

### 步骤5：修改main.c (1分钟)
```c
// User/main.c 顶部添加:
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

// main()函数中LCD初始化后添加:
lcd_init();
lv_init();              // ← 新增
lv_port_disp_init();    // ← 新增
lv_port_indev_init();   // ← 新增

// 创建简单UI:
lv_obj_t *label = lv_label_create(lv_scr_act());
lv_label_set_text(label, "LVGL Works!");
lv_obj_center(label);
```

---

## 🔧 Keil项目配置

**关键步骤**:
1. Project → Manage → Project Items
2. 确保以下文件存在于项目中:
   - `Middlewares/LVGL/GUI/lvgl/porting/lv_port_disp.c` ✅
   - `Middlewares/LVGL/GUI/lvgl/porting/lv_port_indev.c` ✅
   - `Middlewares/LVGL/GUI/lvgl/porting/lv_port_fs.c` (可选)

3. 删除:
   - `Middlewares/TEXT/text.c` ❌
   - `Middlewares/TEXT/fonts.c` ❌

4. 检查包含路径 (Include Paths):
   ```
   .\Middlewares\LVGL\GUI\lvgl\src
   .\Middlewares\LVGL\GUI\lvgl\porting
   ```

---

## 📊 资源占用

| 资源 | TEXT库 | LVGL | 差异 |
|------|--------|------|------|
| SRAM | 15KB | 45KB | +30KB |
| Flash | 35KB | 600KB | 代替TEXT后无额外占用 |
| 功能 | 简单文本 | 完整GUI框架 | 提升100倍 |

✅ **充足** - SRAM剩余150KB可用于音频等

---

## 🎨 最小UI示例

```c
void create_minimal_ui(void)
{
    lv_obj_t *scr = lv_scr_act();
    
    // 清空
    lv_obj_clean(scr);
    
    // 标题
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Music Player");
    lv_obj_set_pos(title, 10, 10);
    
    // 信息
    lv_obj_t *info = lv_label_create(scr);
    lv_label_set_text_fmt(info, "Now Playing: %d", song_index);
    lv_obj_set_pos(info, 10, 50);
    
    // 按钮
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 60, 30);
    lv_obj_set_pos(btn, 130, 100);
    
    lv_obj_t *btn_txt = lv_label_create(btn);
    lv_label_set_text(btn_txt, "Play");
}
```

---

## ❌ 必删文件

```
❌ Middlewares/TEXT/text.c      (35KB)
❌ Middlewares/TEXT/text.h
❌ Middlewares/TEXT/fonts.c     (可选)
❌ Middlewares/TEXT/fonts.h     (可选)

删除前: 在main.c中替换所有 text_show_string() 调用
删除后: 在Keil中移除这些文件的编译
```

---

## ⚠️ 不要删除

```
✅ Middlewares/LVGL/GUI/lvgl/src/      (核心库)
✅ Middlewares/LVGL/GUI/lvgl/porting/  (Port驱动)
✅ Drivers/BSP/LCD/                    (LCD驱动)
✅ Drivers/BSP/KEY/                    (按键驱动)
✅ FreeRTOS/                           (系统)
```

---

## 🐛 常见错误

| 错误 | 原因 | 解决 |
|------|------|------|
| "undefined reference to lv_init" | lv_conf.h 还是 #if 0 | 改为 #if 1 |
| "infinite loop in disp_flush" | LCD驱动函数不存在 | 检查lcd.c是否在项目中 |
| "LCD花屏" | Port驱动的LCD调用错误 | 检查lcd_wr_data(), lcd_set_window() |
| "按键无效" | Port的key_scan()返回值错误 | 检查按键驱动初始化 |
| "Flash超限 >1MB" | 演示程序+TEXT库占空间 | 删除Middlewares/LVGL/GUI_APP/demos/ |

---

## ✅ 成功标志

✓ 编译无错误  
✓ LCD显示LVGL内容  
✓ 按键可控制LVGL焦点  
✓ 无花屏或乱码  
✓ Flash使用不超过1MB  

---

## 📚 详细文档

- **集成步骤**: 见 `LVGL_INTEGRATION_GUIDE.md`
- **删除指南**: 见 `LVGL_FILES_DELETION_GUIDE.md`
- **原作文档**: `CLAUDE.md`

---

## 🚀 支持的LVGL功能

- ✅ **Widgets**: Label, Button, Slider, Bar, Switch等
- ✅ **布局**: Flex, Grid自动布局
- ✅ **动画**: 淡出、缩放、平移等
- ✅ **主题**: 可自定义样式和颜色
- ✅ **国际化**: UTF-8支持中文
- ✅ **输入**: 键盘按键导航
- ⏳ **触摸**: 暂未适配（留作后续)

---

## 📝 代码快速参考

```c
// 初始化
lv_init();
lv_port_disp_init();
lv_port_indev_init();

// 创建对象
lv_obj_t *obj = lv_label_create(parent);

// 设置文本
lv_label_set_text(obj, "Hello");

// 对齐
lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);

// 设置大小
lv_obj_set_size(obj, width, height);

// 设置颜色
lv_obj_set_style_bg_color(obj, lv_color_hex(0xFF0000), 0);

// 定时器处理 (FreeRTOS任务中)
lv_timer_handler();
```

---

**开始前必读**: `LVGL_INTEGRATION_GUIDE.md`

**预计花费时间**: 10-30分钟

**难度等级**: ⭐⭐ (中等)
