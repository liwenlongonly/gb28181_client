#!/bin/bash

#出现错误即退出执行
set -e

. ./config.sh

root_dir=$(pwd)
cd ${root_dir}

# 保活程序
declare -A keepalive=(["target"]="./keepalive.sh" ["path"]="./")
target_map["keepalive"]=keepalive

__gettargetpath(){
    app_name=$1
    for element in ${target_names[@]}
    do
      ret=$(expr match "${app_name}" "${element}")
      if (( ret>0 )); then
        echo ${element}
      fi
    done
}

#检查服务app是否运行
__isrunning() {
    app=$1
    run=$(ps -ef |grep -w $app |grep -v "grep" | grep -v "aitsmanage")
    echo ${run}
}

#获取服务app运行状态
__getruninfo() {
    app=$1
    run=( $( __isrunning $app ) )
    if [ "$run" ]; then
        echo "$app 已启动"
    else
        echo "$app ---> 未启动 <---"
    fi
}

#打开服务目录app_path下的服务app
__startapp() {
  app=$1
	app_path=$2
  sleep=$3
	cd $root_dir
	run=( $( __isrunning $app ) )
	if [ "$run" ]; then
	  echo "$app 已启动"
	else
	  cd $app_path
    nohup ./$app > ./nohup.log 2>&1 &
    echo "$app 启动完成"
    sleep $sleep
	fi
}

#关闭服务
__stopapp() {
	app=$1
	sleep=$2
	pid=$(ps -ef| grep -w $app | grep -v "grep" | grep -v "aitsmanage"|awk '{print $2}')
	if [ "$pid" != "" ]; then
		kill -15 ${pid}
		echo "$app killed pid:$pid"
		sleep $sleep
	fi
}

#获取所有服务运行状态
__statusapp() {
  for element in ${target_names[@]}
  do
    item=${target_map[$element]}
    if [ ${item} ]; then
      target=$( eval echo '${'${item}'["target"]}' )
      __getruninfo $target
    else
      echo "输入的app未找到"
    fi
  done
}

# 获取某个服务的状态
__statusoneapp() {
  item=${target_map[$1]}
  if [ ${item} ]; then
    target=$( eval echo '${'${item}'["target"]}' )
    __getruninfo $target
  else
    echo "输入的app未找到"
  fi
}

__createsoftconnect(){
  app=$1
  app_path=$2
  cd $root_dir
  cd $app_path
  target=${app%"$target_postfix"}
  ln -srf $target $app
  echo $app
}

start(){
  item=${target_map[$1]}
  if [ ${item} ]; then
      target=$( eval echo '${'${item}'["target"]}' )
      path=$( eval echo '${'${item}'["path"]}' )
      __startapp $target $path 0
  else
    echo "输入的app未找到"
  fi
}

stop(){
  item=${target_map[$1]}
  if [ ${item} ]; then
      target=$( eval echo '${'${item}'["target"]}' )
      __stopapp $target 0
  else
    echo "输入的app未找到"
  fi
}

start_all() {
  for element in ${target_names[@]}
  do
    item=${target_map[$element]}
    if [ ${item} ]; then
        target=$( eval echo '${'${item}'["target"]}' )
        path=$( eval echo '${'${item}'["path"]}' )
        __startapp $target $path 3
    else
      echo "输入的app未找到"
    fi
  done
	__statusapp
}

stop_all() {
  #${#array[@]}获取数组长度用于循环
  for(( i=(${#target_names[@]}-1);i>=0;i--)) do
    element=${target_names[i]};
    item=${target_map[$element]}
      if [ ${item} ]; then
          target=$( eval echo '${'${item}'["target"]}' )
          __stopapp $target 3
      else
        echo "输入的app未找到"
    fi
  done;
	__statusapp
}

status_all() {
	__statusapp
}

create_soft_connect(){
  for element in ${target_names[@]}
  do
    item=${target_map[$element]}
    if [ ${item} ]; then
        target=$( eval echo '${'${item}'["target"]}' )
        path=$( eval echo '${'${item}'["path"]}' )
        __createsoftconnect $target $path
    else
      echo "输入的app未找到"
    fi
  done
}

case "$1" in
    start)
      if [ "$2" ]; then
        if [ $2 = "all" ]; then
          start_all
        else
          start $2
        fi
      else
        echo "请输入appname 或 all"
      fi
       ;;
	  stop)
	    if [ "$2" ]; then
	      if [ $2 = "all" ]; then
	        stop_all
	      else
	        stop $2
	      fi
      else
        echo "请输入appname 或 all"
      fi
	    ;;
    stop_all)
       stop_all
       ;;
    restart)
        if [ "$2" ]; then
          if [ $2 = "all" ]; then
            stop_all
            start_all
          else
            stop $2
            sleep 4
            start $2
          fi
        else
            echo "请输入appname 或 all"
          fi
        ;;
    status)
       if [ "$2" ]; then
         if [ $2 = "all" ]; then
           status_all
         else
           __statusoneapp $2
         fi
       else
           echo "请输入appname 或 all"
       fi
       ;;
    soft)
      create_soft_connect
      ;;
    *)
       echo "Usage: $0 {start|stop|status|restart|soft}"
esac

exit 0 
