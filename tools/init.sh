#!/bin/bash

#出现错误即退出执行
set -e

WORKSPACE=`pwd`
cd $WORKSPACE/

VERSION=4.1.0

# libosip2
tar xvf libosip2-$VERSION.tar.gz
cd libosip2-$VERSION

./configure --prefix=$WORKSPACE
make install

cd $WORKSPACE/
rm -rf libosip2-$VERSION

# libexosip2
tar xvf libeXosip2-$VERSION.tar.gz
cd libeXosip2-$VERSION

./configure --prefix=$WORKSPACE LDFLAGS="-L$WORKSPACE/lib" CFLAGS="-L$WORKSPACE/include"
make install

cd $WORKSPACE/
rm -rf libeXosip2-$VERSION