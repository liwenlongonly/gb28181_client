#!/bin/bash
#出现错误即退出执行
set -e

debug=${debug:-1}

#Define target postfix for debug, Such as using your own name
target_postfix=${target_postfix:-''}

# 下面配置用于程序控制
declare -A target_map

# target: 目标名称 path: 目标所在路径
declare -A gb=(["target"]="gb28181-client${target_postfix}" ["path"]="./bin")
target_map["gb"]=gb

target_names=("gb")

