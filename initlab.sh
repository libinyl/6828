#!/usr/bin/env bash

workpath="${HOME}/6828"

if [[ -d ${workpath} ]];then
    echo "path ${workpath} already exists, exit."
    exit 1
fi

mkdir ${workpath}
cd ${workpath}
git clone https://gitee.com/libinyl/6.828-qemu.git qemu
git clone https://pdos.csail.mit.edu/6.828/2018/jos.git lab
