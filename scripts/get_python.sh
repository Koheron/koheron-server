#!/bin/bash

python_path=$1
base_dir=$2

# Determine whether absolute or relative path
if [[ ${python_path} = /* ]]; then
	echo ${python_path}
else
	echo ${base_dir}/${python_path}
fi