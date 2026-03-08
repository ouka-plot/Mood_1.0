# Mood_1.0 项目 - Docker编译指南

## 📌 项目配置
- **MCU**: STM32F407ZGTx (Cortex-M4, 168MHz)
- **Toolchain**: gcc-arm-none-eabi-10.3-2021.10
- **OS**: FreeRTOS
- **GUI**: LVGL
- **Output**: Mood_1.0.bin / Mood_1.0.hex

---

## 🚀 快速开始 (3步)

### 步骤1：构建Docker镜像
```bash
cd d:\desktop\Mood_1.0
docker build -t mood-compiler:latest .
```

### 步骤2：编译项目
```bash
docker run --rm -v "%cd%":/workspace mood-compiler:latest
```

或使用脚本：
```bash
./build.sh          # Linux/Mac
build.bat           # Windows
```

### 步骤3：查看输出
```
Output/
├── Mood_1.0.elf    ✅ 主文件
├── Mood_1.0.bin    ✅ 烧录文件（二进制）
└── Mood_1.0.hex    ✅ 烧录文件（十六进制）
```

---

## 📂 文件结构

```
Mood_1.0/
├── Dockerfile              ← Docker镜像定义
├── Makefile                ← 编译规则
├── build.sh                ← Linux/Mac编译脚本
├── build.bat               ← Windows编译脚本
├── stm32_linker.ld         ← 链接脚本 (由Keil导出)
├── Drivers/                ← STM32驱动
├── FreeRTOS/               ← RTOS核心
├── Middlewares/            ← LVGL等中间件
├── Projects/               ← Keil项目文件
├── User/                   ← 应用代码
└── Output/                 ← 编译输出
```

---

## 🔧 注意事项

### 1. Keil导出链接脚本
Docker编译需要链接脚本(.ld文件)：
```bash
# Keil中：
Project → Options for Target → Linker → Linker Control File
# 找到: Output/Listings/STM32F407IIT6_SRAM.scf
# 转换为GNU LD格式，保存为: stm32_linker.ld
```

### 2. STM32启动文件
确保存在：`Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s`

### 3. 编译优化
默认使用 `-O2` 优化，可在Makefile中调整：
```makefile
OPT = -O2          # 改为-O0(无优化), -O1, -O3
```

---

## 🐳 Docker使用详解

### 方法1：一次性编译
```bash
docker run --rm -v "$(pwd)":/workspace mood-compiler:latest make clean all
```

### 方法2：交互式编译 (调试)
```bash
docker run -it --rm -v "$(pwd)":/workspace mood-compiler:latest bash
# 进入容器后执行:
cd /workspace
make clean
make all
```

### 方法3：使用docker-compose (推荐)
创建 `docker-compose.yml`:
```yaml
version: '3'
services:
  build:
    build: .
    volumes:
      - .:/workspace
    working_dir: /workspace
    command: make clean all
```

执行：
```bash
docker-compose up
```

---

## 📊 编译输出示例

```
$ make clean all
Removing old files...
Compiling C files...
[CC] startup_stm32f407xx.s
[CC] system_stm32f4xx.c
[CC] stm32f4xx_it.c
[CC] main.c
[CC] audioplay.c
... (200+ files)
Linking...
[LD] Output/Mood_1.0.elf
Generating hex/bin...
[OBJCOPY] Output/Mood_1.0.hex
[OBJCOPY] Output/Mood_1.0.bin

Build complete!
   Mood_1.0.elf  (ELF format, for debugger)
   Mood_1.0.hex  (Intel HEX format)
   Mood_1.0.bin  (Binary format)

Size report:
   text      data       bss       dec       hex   filename
 123456      1234      4096    128786    1F6B2   Output/Mood_1.0.elf
```

---

## ✅ Keil和Docker双编译

### Windows工作流：
```
开发:           日常编码 + 用Keil IDE测试 (Project → Build)
    ↓
CI/CD:         推送到Git后，自动Docker编译验证
    ↓
发布:           从Docker编译的版本进行量产烧录
```

### 添加Git Hook自动编译
创建 `.git/hooks/pre-commit`:
```bash
#!/bin/bash
echo "Building with Docker..."
docker run --rm -v "$(pwd)":/workspace mood-compiler:latest make clean all
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi
```

---

## 🔍 常见问题

### Q: Docker编译失败？
A:
1. 检查Dockerfile中的工具链路径是否匹配
2. 检查所有必需的源文件是否存在
3. 进交互式容器调试：`docker run -it --rm -v "$(pwd)":/workspace mood-compiler:latest bash`

### Q: Keil项目和Docker编译产物不一样？
A: 
- 检查编译参数是否一致 (O2优化、MCU型号等)
- 检查链接脚本(.ld)配置
- 用objdump对比符号表：`arm-none-eabi-objdump -t output1.elf output2.elf`

### Q: 编译很慢？
A: 使用并行编译：
```bash
docker run --rm -v "$(pwd)":/workspace mood-compiler:latest make -j4 all
```

---

## 📦 所需文件清单

- [x] Dockerfile (ST/ARM编译环境)
- [x] Makefile (编译规则)
- [x] build.sh (Linux/Mac启动脚本)
- [x] build.bat (Windows启动脚本)
- [ ] stm32_linker.ld (需要从Keil导出)
- [x] 源代码 (已存在)

---

## 🎯 下一步

1. **立即可做**:
   - [x] 已创建Dockerfile
   - [x] 已创建Makefile
   - [x] 已创建build脚本

2. **需要你做**:
   - [ ] 从Keil导出链接脚本 (.ld文件)
   - [ ] 运行 `docker build` 构建镜像
   - [ ] 运行 `docker run` 执行编译

3. **可选**:
   - [ ] 配置GitHub Actions自动编译
   - [ ] 配置编译输出到云存储
   - [ ] 集成自动化测试

---

**准备开始了吗？** 👇

1. 构建Docker镜像 → `docker build -t mood-compiler:latest .`
2. 编译项目 → `docker run --rm -v "$(pwd)":/workspace mood-compiler:latest`
3. 检查Output目录中的.elf/.bin/.hex文件
