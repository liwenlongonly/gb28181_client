#!/bin/bash
#出现错误即退出执行
set -e
#创建软连接脚本
#------------------
root=$(pwd)
cd ${root}

if [ ! -d "./linux_lib/" ];then
    mkdir ./linux_lib
else
    echo "文件夹已经存在"
fi

\cp ./x64/* ./linux_lib
cd ./linux_lib

#----------------------------------
# ssl
#----------------------------------
ln -snf libssl.so.10 libssl.so
ln -snf libcrypto.so.10 libcrypto.so

#----------------------------------
# osip
#----------------------------------
ln -snf libosipparser2.so.11.0.0 libosipparser2.so.11
ln -snf libosipparser2.so.11 libosipparser2.so

ln -snf libosip2.so.11.0.0 libosip2.so.11
ln -snf libosip2.so.11 libosip2.so

#----------------------------------
# eXosip2
#----------------------------------
ln -snf libeXosip2.so.11.0.0 libeXosip2.so.11
ln -snf libeXosip2.so.11 libeXosip2.so
