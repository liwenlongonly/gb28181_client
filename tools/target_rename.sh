#!/bin/bash
target_name=$1
target_version=$2
install_dir=$3
echo "${target_name} ${target_version} ${install_dir}"
cd ${install_dir}
mv ${target_name} "${target_name}_${target_version}"
ln -srf "${target_name}_${target_version}" ${target_name}