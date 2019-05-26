#!/usr/bin/env bash
workpath="${HOME}/6828"

if [[ -d ${workpath} ]];then
    read -p "path \"${workpath}\" already exists. To force init,press 'f'; 'e' or others to exit." f1
    case $f1 in
    'f')    rm -rf ${workpath}  break   ;;
     * )    echo "exit."        exit 0  ;;
    esac

fi


mkdir   ${workpath}
cd      ${workpath}

read -p "where do you prefer to download the qemu code? press 'p' for personal; 'o' or others for offical(recommanded if your network is not good):" f2
case $f2 in
    "p") qemupath="https://gitee.com/libinyl/6.828-qemu.git"    ;;
     * ) qemupath="https://github.com/mit-pdos/6.828-qemu.git"  ;;
esac

labpath="https://pdos.csail.mit.edu/6.828/2018/jos.git"

# clone qemu code.
echo "start to clone qemu code..."
git clone ${qemupath} qemu

if [[ $? -ne 0 ]]; then
    echo "clone qemu code failed.exit."
    exit 0;
else
    echo "clone qemu code done."
fi


# clone lab code
echo "start to clone lab code..."

git clone ${labpath} lab
if [[ $? != 0 ]]; then
    echo "clone lab code failed.exit."
    exit 0;
else
    echo "clone lab code done."
fi


echo "init all lab code done."
