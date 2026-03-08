# LVGL 移植方案总结

## 现状分析

你的项目已有：
- ✅ LVGL库已存在 (`Middlewares/LVGL/`)  
- ✅ Port文件模板已存在 (`Middlewares/LVGL/GUI/lvgl/porting/`)
- ✅ LCD驱动已完成 (FSMC 8080，320×240)
- ✅ 按键驱动已完成
- ❌ LVGL未启用 (所有配置为 `#if 0`)
- ❌ Port文件未集成到项目

---

## 📋 需要做的事（任务清单）

### 第1阶段：启用LVGL配置 (5分钟)

**文件**: `Middlewares/LVGL/GUI/lvgl/lv_conf.h`

```diff
- #if 0 /*Set it to "1" to enable content*/
+ #if 1 /*Set it to "1" to enable content*/

  // 改以下参数:
- #define LV_MEM_SIZE (48U * 1024U)
+ #define LV_MEM_SIZE (32U * 1024U)      // 减小到32KB

- #define LV_MEM_BUF_MAX_NUM 16
+ #define LV_MEM_BUF_MAX_NUM 8           // 减小到8

  // 确保分辨率正确（已是默认):
  #define LV_HOR_RES_MAX 320             // ✅ 正确
  #define LV_VER_RES_MAX 240             // ✅ 正确
  #define LV_COLOR_DEPTH 16              // ✅ 正确
```

---

### 第2阶段：启用Port文件 (5分钟)

**4个Port文件都需要改 `#if 0` → `#if 1`**

| 文件 | 路径 | 第7行改动 |
|------|------|----------|
| lv_port_disp.h | `porting/` | `#if 0` → `#if 1` |
| lv_port_disp.c | `porting/` | `#if 0` → `#if 1` |
| lv_port_indev.h | `porting/` | `#if 0` → `#if 1` |
| lv_port_indev.c | `porting/` | `#if 0` → `#if 1` |

**另外：** `lv_port_fs.h` / `.c` - 暂不需要启用（这是文件系统适配）

---

### 第3阶段：适配Port驱动到你的LCD和KEY (10分钟)

#### 3.1 LCD驱动适配 (`lv_port_disp.c`)

找到 `disp_flush()` 函数，修改为调用你项目的LCD函数：

**改前** (模板默认):
```c
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    // TODO: 这里写你的LCD驱动代码
}
```

**改后** (你的项目):
```c
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    uint16_t x1 = area->x1, y1 = area->y1;
    uint16_t x2 = area->x2, y2 = area->y2;
    
    // 调用你项目的LCD函数
    lcd_set_window(x1, y1, x2-x1+1, y2-y1+1);
    lcd_write_ram_prepare();
    
    uint32_t size = (x2-x1+1) * (y2-y1+1);
    uint16_t *buf = (uint16_t *)color_p;
    for(uint32_t i = 0; i < size; i++) {
        lcd_wr_data(buf[i]);  // 你项目中的LCD函数
    }
    
    lv_disp_flush_ready(disp_drv);
}
```

#### 3.2 按键驱动适配 (`lv_port_indev.c`)

找到 `indev_read_cb()` 函数，修改为调用你的按键函数：

**改前** (模板默认):
```c
static void indev_read_cb(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    // TODO: 这里写你的按键读取代码
    data->state = LV_INDEV_STATE_RELEASED;
}
```

**改后** (你的项目):
```c
static void indev_read_cb(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    uint8_t key = key_scan(0);  // 你项目的按键函数
    
    if (key == KEY0_PRES) {
        data->key = LV_KEY_RIGHT;       // 下一个
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (key == KEY2_PRES) {
        data->key = LV_KEY_LEFT;        // 上一个
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (key == KEY1_PRES) {
        data->key = LV_KEY_ENTER;       // 确认
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (key == KEY_UP_PRES) {
        data->key = LV_KEY_ESC;         // 返回
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
```

---

### 第4阶段：修改 main.c 集成LVGL (5分钟)

**文件**: `User/main.c`

#### 4.1 添加头文件（顶部）:
```c
// 添加
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

// 删除或注释
// #include "./TEXT/text.h"
```

#### 4.2 在 main() 函数中初始化（在LCD初始化后）:
```c
int main(void)
{
    // ... 其他初始化代码 ...
    
    lcd_init();                 // LCD初始化（原有的）
    
    /* NEW: LVGL初始化 */
    lv_init();                  // 初始化LVGL
    lv_port_disp_init();        // 初始化显示驱动
    lv_port_indev_init();       // 初始化输入驱动
    
    /* 创建简单UI (可选) */
    lv_obj_t *scr = lv_scr_act();
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "LVGL Ready!");
    lv_obj_center(label);
    
    // ... 创建FreeRTOS任务 ...
    vTaskStartScheduler();
    
    while(1);
}
```

#### 4.3 删除TEXT库调用（搜索并替换）:

**搜索**: `text_show_string`

**在main.c中**，将类似代码：
```c
text_show_string(30, 30, 200, 16, "正点原子STM32开发板", 16, 0, RED);
text_show_string(30, 50, 200, 16, "音乐播放器实验", 16, 0, RED);
```

**替换为**:
```c
lv_obj_t *title = lv_label_create(lv_scr_act());
lv_label_set_text(title, "STM32 Music Player");
lv_obj_align(title, LV_ALIGN_TOP_LEFT, 30, 30);

lv_obj_t *info = lv_label_create(lv_scr_act());
lv_label_set_text(info, "Audio Player Demo");
lv_obj_align(info, LV_ALIGN_TOP_LEFT, 30, 50);
```

---

### 第5阶段：配置FreeRTOS自动更新 (3分钟)

**文件**: `User/FreeRTOSConfig.h`

```c
// 搜索 configUSE_TICK_HOOK，改为:
#define configUSE_TICK_HOOK 1
```

这样FreeRTOS会自动每1ms调用 `vApplicationTickHook()` 来更新LVGL时钟。

---

### 第6阶段：Keil项目配置 (5分钟)

#### 6.1 添加Port源文件

**Project → Manage → Project Items**

在 "User" 或适当分组中，**确保包含**:
- `Middlewares/LVGL/GUI/lvgl/porting/lv_port_disp.c`  ✅
- `Middlewares/LVGL/GUI/lvgl/porting/lv_port_indev.c` ✅

#### 6.2 删除TEXT库源文件

在同一菜单中，**删除或禁用**:
- `Middlewares/TEXT/text.c` ❌
- `Middlewares/TEXT/fonts.c` ❌

#### 6.3 检查包含路径

**Project → Options for Target → C/C++**

**Include Paths** 应包含:
```
.\Middlewares\LVGL\GUI\lvgl\src
.\Middlewares\LVGL\GUI\lvgl\porting
.\User
```

---

### 第7阶段：编译和测试 (5分钟)

**编译**:
```
Project → Build Target (Ctrl+F7)
```

预期结果：
- ✅ 0 errors
- ⚠️ 若干 warnings (正常)

**下载**:
```
Flash → Download
```

**测试**:
- 复位开发板
- LCD应显示 "LVGL Ready!"
- 按KEY0/KEY2应能导航LVGL
- 无花屏或乱码

---

## 🗑️ 可删除的文件（可选，但推荐）

### 必删：TEXT库 (节省35KB)
```
❌ Middlewares/TEXT/text.c
❌ Middlewares/TEXT/text.h
❌ Middlewares/TEXT/fonts.c
❌ Middlewares/TEXT/fonts.h
```

删除步骤：
1. 在Keil项目中移除这些文件
2. 从 `User/main.c` 删除 `#include "./TEXT/text.h"`
3. 删除所有 `text_show_string()` 调用（用LVGL替代）
4. 物理删除文件（可选）

### 可选删：演示程序 (节省200KB)
```
ⓘ Middlewares/LVGL/GUI_APP/demos/
ⓘ Middlewares/LVGL/examples/
```

只有在Flash空间不足时才删除。

---

## ⚡ 快速执行计划

```
总耗时: 30-45分钟

第1步 (5min):  编辑lv_conf.h
               → 改#if 0为#if 1
               → 调整内存参数

第2步 (5min):  启用4个Port文件
               → 都改#if 0为#if 1

第3步 (10min): 适配Port驱动
               → disp_flush()调用LCD函数
               → indev_read_cb()调用key_scan()

第4步 (5min):  修改main.c
               → 添加头文件
               → 添加初始化代码
               → 删除TEXT库调用

第5步 (3min):  FreeRTOSConfig.h
               → 启用configUSE_TICK_HOOK

第6步 (5min):  Keil项目配置
               → 添加Port源文件
               → 删除TEXT源文件
               → 检查包含路径

第7步 (5min):  编译和测试
               → Build成功
               → Download成功
               → LCD显示验证
```

---

## 📚 参考文档

本项目已为你生成以下文档：

| 文档 | 内容 | 推荐阅读场景 |
|------|------|-----------|
| `LVGL_QUICK_REFERENCE.md` | 快速参考卡，所有关键代码 | 👈 **先读** |
| `LVGL_INTEGRATION_GUIDE.md` | 详细集成步骤，每步说明 | 遇到问题时查阅 |
| `LVGL_FILES_DELETION_GUIDE.md` | TEXT库删除指南，完整清单 | 删除文件时使用 |
| `CLAUDE.md` | 项目原概述文档 | 了解项目背景 |

---

## ✅ 完成标志

当你完成上述所有步骤后，应该看到：

- [x] 编译成功（0 errors）
- [x] LCD显示LVGL内容
- [x] 按键可控制UI焦点
- [x] 无花屏或乱码
- [x] Flash使用不超过1.2MB
- [x] SRAM使用不超过100KB

---

## 🆘 如出现问题

| 症状 | 原因 | 解决 |
|------|------|------|
| 编译错误："undefined reference to lv_init" | lv_conf.h还是#if 0 | 改为#if 1 |
| LCD花屏乱码 | disp_flush()调用的LCD函数错误 | 检查lcd.c中的函数名 |
| 按键无反应 | indev_read_cb()错误或未调用 | 检查key_scan()返回值 |
| Flash超限 | 演示程序+TEXT库占空间 | 删除demos目录 |
| RAM不足 | LV_MEM_SIZE设置过大 | 改为24KB试试 |

详见 `LVGL_INTEGRATION_GUIDE.md` 的"常见问题"章节。

---

**祝你移植顺利！🎉**

按照上述步骤按序完成，预计30-45分钟内可完成整个集成。
