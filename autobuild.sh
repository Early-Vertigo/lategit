#!/bin/bash

set -e
# 删除当前build目录下的所有文件
rm -rf `pwd`/build/*
# 进入build后camke make
cd `pwd`/build &&
    cmake .. &&
    make
# 退出到根目录
cd ..
# 将include拷贝到lib中
cp -r `pwd`/src/include `pwd`/lib