#! /usr/bin/python

# (c) Koheron

import os
import sys
import yaml
import jinja2
import time
import subprocess
from shutil import rmtree, copyfile, copytree

from devgen import Device, device_table

SRC_DIR = 'tmp'
DEV_DIR = SRC_DIR + '/devices'

def init_src_dir(middleware_path):
    if os.path.isdir(SRC_DIR):
        rmtree(SRC_DIR)

    copytree('core', os.path.join(SRC_DIR, 'core'))
    os.remove(os.path.join(SRC_DIR, 'core/Makefile'))
    os.remove(os.path.join(SRC_DIR, 'core/main.cpp'))
    copytree(middleware_path, os.path.join(SRC_DIR, 'middleware'))
    copyfile('core/main.cpp', os.path.join(SRC_DIR, 'main.cpp'))
    os.makedirs(DEV_DIR)

def render_makefile(obj_files):
    print "Generate Makefile"
    makefile_renderer = jinja2.Environment(
      loader = jinja2.FileSystemLoader(os.path.abspath('.'))
    )

    template = makefile_renderer.get_template('core/Makefile')
    date = time.strftime("%c")
    makefile_filename = os.path.join(SRC_DIR, 'Makefile')

    output = file(makefile_filename, 'w')
    output.write(template.render(date=date, objs_list=obj_files))
    output.close()

def render_device_table(devices):
    print "Generate device table"
    device_table.PrintDeviceTable(devices, DEV_DIR)
    template_filename = 'devgen/templates/devices.hpp'

    header_renderer = jinja2.Environment(
      loader = jinja2.FileSystemLoader(os.path.abspath('.'))
    )

    template = header_renderer.get_template(template_filename)
    header_filename = os.path.join(DEV_DIR, 'devices.hpp')
    output = file(header_filename, 'w')
    output.write(template.render(devices=devices))
    output.close()

def generate(devices_list, base_dir):
    devices = [] # List of generated devices
    obj_files = []  # Object file names

    for path in devices_list:       
        device = Device(path, base_dir)
        
        print "Generate " + device.name
        device.Generate(DEV_DIR)
        devices.append(device)
        obj_files.append(os.path.join('devices', device.class_name.lower()+'.o'))

    return devices, obj_files

def compile(config, middleware_path):
    print "Compiling ..."

    if "host" in config:
        subprocess.check_call("make -C " + SRC_DIR + " TARGET_HOST=" + config["host"] + " clean all", shell=True)
    elif "cross-compile" in config:
        subprocess.check_call("make -C " + SRC_DIR + " CROSS_COMPILE=" + config["cross-compile"] + " clean all", shell=True)
    else:
        raise ValueError("Specify a target host or a cross-compilation toolchain")

def main(argv):
    with open(argv[0]) as config_file:    
        config = yaml.load(config_file)

    init_src_dir(argv[1])
    devices, obj_files = generate(config["devices"], os.path.dirname(argv[0]))
    render_makefile(obj_files)
    render_device_table(devices)
    compile(config, argv[1])

if __name__ == "__main__":
        main(sys.argv[1:])
