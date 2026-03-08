# STM32F407 LVGL 音乐播放器 - 优化编译环境
# 基于: gcc-arm-none-eabi-10.3-2021.10 (与GDF4项目一致)
# 改进: 在镜像构建时下载工具链，避免将大体积压缩包提交到仓库

FROM ubuntu:20.04

# 避免交互式安装
ENV DEBIAN_FRONTEND=noninteractive

# 1. 安装基础工具和编译依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip \
    curl \
    bzip2 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# 2. 下载工具链并解压
WORKDIR /opt

RUN curl -L -o gcc-arm-none-eabi.tar.bz2 \
        https://developer.arm.com/-/media/Files/downloads/gnu/10.3-2021.10/binrel/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 \
    && tar -xjf gcc-arm-none-eabi.tar.bz2 \
    && rm gcc-arm-none-eabi.tar.bz2

# 3. 设置工具链路径
ENV PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin:${PATH}"

# 4. 验证工具链
RUN arm-none-eabi-gcc --version && \
    arm-none-eabi-g++ --version && \
    arm-none-eabi-objcopy --version && \
    arm-none-eabi-objdump --version && \
    arm-none-eabi-size --version

# 5. 进入项目工作目录
WORKDIR /workspace

# 6. 默认编译命令
CMD ["make", "clean", "all"]
