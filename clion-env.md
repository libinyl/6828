# 6.828 MacOS + CLion 本地调试环境搭建

## 1 安装 qemu

   
```
cd ~
git clone https://github.com/libinyl/6.828-qemu qemu
xcode-select --install
brew install $(brew deps qemu) # 安装qemu 依赖项
brew install pkg-config

# 配置 qemu.如果提示 python 版本不正确,则加上 --python=/usr/bin/python
cd qemu
./configure --disable-kvm --disable-werror --disable-sdl --target-list="i386-softmmu x86_64-softmmu"
# 编译,安装 qemu
PATH=${PATH}:/usr/local/opt/gettext/bin make install 
```

## 2 下载 6.828 实验代码

```
git clone git clone https://pdos.csail.mit.edu/6.828/2018/jos.git lab
```

## 3 配置交叉编译环境

### 3.1 安装配置 macport

下载安装 macport 后在`/etc/paths`中加上两行:

```
/opt/local/bin
/opt/local/sbin
```

### 3.2 下载安装 elf32 toolchain

```
port install i386-elf-gcc
```

### 3.3 替换 .gdbinit

把本仓库中的`.gdbinit-clion`复制到用户目录下并更名为`.gdbinit`.

**一定要把 .gdbinit**中的`source path`设置正确,否则 clion 无法正常调试.

## 4 配置 CLion

添加调试设置,gdb remote debug,打断点调试即可.

## 5 最终效果

![clion 调试环境](/images/clion-env.jpg)