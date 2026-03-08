# LVGL适配 - 可删除文件清单

## 📌 项目硬件信息
- **LCD**: 2.8寸电容触摸屏，FSMC驱动，320×240分辨率
- **接口**: 8080并行接口（MCU屏）
- **触摸**: 暂未适配（留作后续扩展）

---

## ❌ 必须删除的文件（TEXT库替代）

### TEXT库相关文件

| 文件路径 | 大小官档 | 删除原因 | 风险等级 |
|---------|---------|---------|---------|
| `Middlewares/TEXT/text.c` | ~20KB | LVGL完全替代 | 🟢 低 |
| `Middlewares/TEXT/text.h` | `<1KB | 头文件 | 🟢 低 |
| `Middlewares/TEXT/fonts.c` | ~15KB | LVGL内置字体 | 🟢 低 |
| `Middlewares/TEXT/fonts.h` | `<1KB | 头文件 | 🟢 低 |

**删除前检查清单**：
- [ ] 在 `User/main.c` 中搜索 `text_show_string` 调用 → 全部替换为LVGL代码
- [ ] 搜索 `#include "./TEXT/text.h"` → 注释或删除
- [ ] 搜索 `#include "./TEXT/fonts.h"` → 注释或删除
- [ ] 在Keil项目中删除 `text.c` 和 `fonts.c` 源文件

**删除步骤**：

1. **代码替换** (main.c中的TEXT调用)

   原代码（TEXT库）:
   ```c
   text_show_string(30, 30, 200, 16, "Music Player", 16, 0, RED);
   ```

   新代码（LVGL）:
   ```c
   lv_obj_t *label = lv_label_create(lv_scr_act());
   lv_label_set_text(label, "Music Player");
   lv_obj_align(label, LV_ALIGN_TOP_LEFT, 30, 30);
   ```

2. **删除Keil项目中的源文件**
   - Project → Manage → Project Items
   - 找到并删除/移除:
     - `Middlewares/TEXT/text.c`
     - `Middlewares/TEXT/fonts.c`

3. **物理删除文件** (可选，建议保留备份)
   ```powershell
   # 备份 (可选)
   Copy-Item -Path "Middlewares/TEXT" -Destination "Middlewares/TEXT_backup" -Recurse
   
   # 删除
   Remove-Item -Path "Middlewares/TEXT" -Recurse -Force
   ```

---

## ⓘ 可选删除的文件（示例代码）

### LVGL演示程序（不必需）

| 路径 | 大小 | 删除原因 | 优先级 |
|------|------|---------|--------|
| `Middlewares/LVGL/GUI_APP/demos/` | ~200KB | 示例程序 | **中** |
| `Middlewares/LVGL/examples/` | ~100KB | 示例代码 | 低 |

**什么时候删除**：
- 🟢 Flash空间充足 → 保留（有价值的参考）
- 🟡 Flash接近1MB限制 → 可删除
- 🔴 Flash严重超限 → 必删

**包含内容**：
```
Middlewares/LVGL/GUI_APP/demos/
├── music/              ← 音乐播放器demo
├── widgets/            ← 控件展示demo
├── stress/             ← 压力测试demo
└── benchmark/          ← 性能测试demo
```

**删除步骤**：

```powershell
# 删除整个demos目录
Remove-Item -Path "Middlewares/LVGL/GUI_APP/demos" -Recurse -Force

# 或删除examples
Remove-Item -Path "Middlewares/LVGL/examples" -Recurse -Force
```

---

## ⚠️ 不要删除的文件（重要）

### LVGL核心库 - 必保留

| 路径 | 用途 | 删除后果 |
|------|------|---------|
| `Middlewares/LVGL/GUI/lvgl/src/` | 核心库 | 🔴 编译失败 |
| `Middlewares/LVGL/GUI/lvgl/lv_conf.h` | 配置文件 | 🔴 编译失败 |
| `Middlewares/LVGL/GUI/lvgl/lv.h` | 主头文件 | 🔴 编译失败 |
| `Middlewares/LVGL/GUI/lvgl/porting/` | Port驱动 | 🔴 编译失败 |

### LCD驱动 - 必保留

| 路径 | 用途 | 删除后果 |
|------|------|---------|
| `Drivers/BSP/LCD/` | LCD FSMC驱动 | 🔴 LVGL无法显示 |
| `Drivers/BSP/KEY/` | 按键驱动 | 🔴 输入无效 |

### FreeRTOS和系统 - 必保留

| 路径 | 用途 | 删除后果 |
|------|------|---------|
| `FreeRTOS/` | RTOS内核 | 🔴 系统崩溃 |
| `Drivers/CMSIS/` | ARM核心库 | 🔴 MCU驱动失效 |
| `Drivers/STM32F4xx_HAL_Driver/` | HAL库 | 🔴 硬件驱动失效 |

---

## 🔧 删除TEXT库后的编译文件清理

Keil编译后会生成大量临时文件，可以清理以释放空间：

```powershell
# 方法1：使用Keil菜单
# Project → Clean (自动删除Output中的.o和.d文件)

# 方法2：手动清理Output目录中不必要的文件
Remove-Item -Path "Output/*.o" -Force
Remove-Item -Path "Output/*.d" -Force
Remove-Item -Path "Output/*.crf" -Force
```

---

## 📊 删除前后的大小对比

### Flash空间

```
删除TEXT库前 (完整版):
├── 应用代码     :  100KB
├── FreeRTOS    :  200KB
├── HAL驱动     :  300KB
├── FATFS       :  200KB
├── 音频解码    :  100KB
├── TEXT库      :   35KB  ❌
├── LVGL库      :  600KB
└── 演示程序    :  200KB  ⓘ
   ━━━━━━━━━━━━━━━━━━━
   总计: 1735KB (超出1MB限制！)

删除TEXT库后 (精简版):
├── 应用代码     :  100KB
├── FreeRTOS    :  200KB
├── HAL驱动     :  300KB
├── FATFS       :  200KB
├── 音频解码    :  100KB
├── LVGL库      :  600KB  ✅
└── 演示程序    :  200KB  ⓘ
   ━━━━━━━━━━━━━━━━━━━
   总计: 1700KB

再删演示程序后 (最精简):
├── 应用代码     :  100KB
├── FreeRTOS    :  200KB
├── HAL驱动     :  300KB
├── FATFS       :  200KB
├── 音频解码    :  100KB
├── LVGL库      :  600KB  ✅
   ━━━━━━━━━━━━━━━━━━━
   总计: 1500KB (充足)
```

### SRAM空间

```
前: TEXT库占5KB
├── TEXT缓冲  : 5KB
└── LCD缓冲   : 10KB
   总计: 15KB

后: LVGL占45KB
├── LVGL内存池 : 32KB
├── LCD缓冲    : 12KB
├── 其他       : 1KB
   总计: 45KB

增加30KB，但功能提升100倍 ✅
```

---

## 📋 删除TEXT库的完整检查清单

### 代码修改
- [ ] 在 `User/main.c` 中注释或删除 `#include "./TEXT/text.h"`
- [ ] 在 `User/main.c` 中搜索所有 `text_show_string()` 调用
- [ ] 逐个替换为LVGL等效代码 (`lv_label_create()`, `lv_label_set_text()`, `lv_obj_align()`)
- [ ] 删除所有 `text.h`, `fonts.h` 的包含
- [ ] 搜索其他.c文件中的TEXT库使用 → 全部替换或删除
- [ ] 编译验证无编译错误

### Keil项目修改
- [ ] 打开 `Projects/MDK-ARM/atk_f407.uvprojx`
- [ ] Project → Manage → Project Items
- [ ] 找到 `Middlewares/TEXT/text.c` → 删除或移除
- [ ] 找到 `Middlewares/TEXT/fonts.c` → 删除或移除
- [ ] 所有包含路径中删除TEXT相关路径 (如有的话)
- [ ] 保存项目

### 文件系统删除
- [ ] 编译测试通过后，删除物理文件 (可选)
  ```
  ❌ Middlewares/TEXT/text.c
  ❌ Middlewares/TEXT/text.h
  ❌ Middlewares/TEXT/fonts.c
  ❌ Middlewares/TEXT/fonts.h
  ```
- [ ] 如果Middlewares/TEXT目录为空，删除整个目录

### 可选：删除演示程序节省200KB

- [ ] 如果Flash空间紧张，删除
  ```
  ❌ Middlewares/LVGL/GUI_APP/demos/
  ```
- [ ] 如果需要参考，保留在备份目录

### 最终验证
- [ ] Project → Build (编译成功)
- [ ] Flash → Download (下载到开发板)
- [ ] 运行验证LVGL UI显示正常
- [ ] 按键控制正常

---

## 🔄 回滚方案（如出现问题）

如果删除TEXT库后出现问题，可以恢复：

```powershell
# 如有备份，恢复
Copy-Item -Path "Middlewares/TEXT_backup/*" -Destination "Middlewares/TEXT" -Recurse

# 在Keil中重新添加text.c和fonts.c
# Project → Manage → Add Files
```

---

## 💡 提示

- **删除TEXT库是可选但推荐的**，因为LVGL完全替代其功能
- **先修改代码再删物理文件**，这样更安全
- **保留backup三个月**，以防急需恢复
- **演示程序可立即删除**，没有依赖关系
- **LVGL核心库不要删除**，否则编译失败

---

## ✅ 完成后的项目结构

```
项目根目录/
├── Drivers/
│   ├── BSP/
│   │   ├── LCD/          ✅ 保留
│   │   ├── KEY/          ✅ 保留
│   │   └── ... (其他)    ✅ 保留
│   └── ... (其他驱动)    ✅ 保留
├── FreeRTOS/             ✅ 保留
├── Middlewares/
│   ├── AUDIOCODEC/       ✅ 保留
│   ├── FATFS/            ✅ 保留
│   ├── LVGL/             ✅ 保留（现在使用）
│   │   ├── GUI/
│   │   │   └── lvgl/
│   │   │       ├── src/  ✅
│   │   │       ├── porting/ ✅ 
│   │   │       └── lv_conf.h ✅
│   │   └── GUI_APP/
│   │       ├── demos/    ⓘ 已删除或保留
│   │       └── examples/ ⓘ 已删除或保留
│   └── TEXT/             ❌ [已删除]
├── Projects/
├── User/                 (LVGL初始化代码)
└── Output/               ✅ 编译输出
```

---

## 📞 常见问题

**Q: 删除TEXT库后编译出错？**
- A: 检查main.c是否还有 `text_show_string()` 调用
- A: 检查包含路径中是否还有TEXT相关路径

**Q: 如何从TEXT库迁移到LVGL？**
参考 [LVGL_INTEGRATION_GUIDE.md](./LVGL_INTEGRATION_GUIDE.md) 的UI示例部分

**Q: 演示程序能保留吗？**
- 可以，但推荐删除以节省200KB Flash空间
- 如要保留，在Keil项目中排除编译即可

