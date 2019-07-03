## linux 常用命令

查看电脑是多少位

`getconf LONG_BIT`

从本地上传到服务器

`scp 源 宿`

## 初始化 qemu

```
cd qemu
./configure --disable-kvm --disable-werror  --target-list="i386-softmmu x86_64-softmmu"
```

- 如果配置 qemu 时缺少包:

```
apt install pkg-config libglib2.0-dev zlib1g-dev libpixman-1-dev
```
