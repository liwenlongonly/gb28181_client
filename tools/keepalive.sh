#!/bin/bash

#出现错误即退出执行
set -e

. ./config.sh

root_dir=$(pwd)
cd ${root_dir}

#检查服务app是否运行
__isrunning() {
    app=$1
    run=$(ps -ef |grep -w $app |grep -v "grep" | grep -v "aitsmanage")
    echo ${run}
}

#打开服务目录app_path下的服务app
__startapp() {
  app=$1
	app_path=$2
  sleep=$3
	cd $root_dir
	run=( $( __isrunning $app ) )
	if [ "$run" ]; then
	  cd ./
	else
	  cd $app_path
    nohup ./$app > ./nohup.log 2>&1 &
    echo "$app 启动完成"
    sleep $sleep
	fi
}

while true
do
	for element in ${target_names[@]}
  do
    item=${target_map[$element]}
    if [ ${item} ]; then
      target=$( eval echo '${'${item}'["target"]}' )
      path=$( eval echo '${'${item}'["path"]}' )
      __startapp $target $path 0
    else
      echo "输入的app未找到"
    fi
  done
	sleep 10
done

exit 0