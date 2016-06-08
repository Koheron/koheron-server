#! /usr/bin/python

# (c) Koheron

import os
import sys
import yaml
import jinja2
import time
import subprocess

from devgen import Device, device_table

TMP = 'tmp'
DEV_DIR = TMP + '/devices'
 
def render_makefile(obj_files):
    print "Generate Makefile"
    makefile_renderer = jinja2.Environment(
      loader = jinja2.FileSystemLoader(os.path.abspath('.'))
    )

    template = makefile_renderer.get_template('core/Makefile')
    date = time.strftime("%c")
    makefile_filename = os.path.join(TMP, 'Makefile')

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


def compile(config):
    arch_flags = '"-' + ' -'.join(config['arch_flags']) + '"'
    defines = '"-D' + ' -D'.join(config['defines']) + '"'
    subprocess.check_call('make -C ' + TMP + ' TARGET_HOST=' + config['host'] 
                                           + ' CROSS_COMPILE=' + config['cross-compile'] 
                                           + ' DEFINES=' + defines
                                           + ' ARCH_FLAGS=' + arch_flags, shell=True)

def main(argv):
    with open(argv[0]) as config_file:    
        config = yaml.load(config_file)

    devices, obj_files = generate(config["devices"], os.path.dirname(argv[0]))
    render_makefile(obj_files)
    render_device_table(devices)
    compile(config)

if __name__ == "__main__":
        main(sys.argv[1:])
