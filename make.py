#! /usr/bin/python

# (c) Koheron

import os
import sys
import yaml
import jinja2
import time
import subprocess
from distutils.dir_util import copy_tree
from shutil import copy

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
    with open(header_filename, 'w') as f:
        f.write(template.render(devices=devices))

def generate(devices_list, midware_path):
    devices = [] # List of generated devices
    obj_files = []  # Object file names

    for path in devices_list:
        if path.endswith('.hpp') or path.endswith('.h'):
            device = Device(path, midware_path)
            print "Generate " + device.name
            device.Generate(os.path.join(midware_path, os.path.dirname(path)))
            devices.append(device)
            obj_files.append(os.path.join('devices', device.class_name.lower()+'.o'))

    return devices, obj_files

def install_requirements(config, base_dir):
    if 'requirements' in config:
        for requirement in config['requirements']:
            if requirement['type'] == 'git':
                subprocess.call(['git', 'clone', requirement['src'], requirement['dest']])
                subprocess.call('cd ' + requirement['dest'] + ' && git checkout ' + requirement['branch'], shell=True)
            elif requirement['type'] == 'folder':
                copy_tree(os.path.join(base_dir, requirement['from'], requirement['import']),
                          os.path.join('tmp/middleware', requirement['import']))
            elif requirement['type'] == 'file':
                dest_dir = os.path.join('tmp/middleware', os.path.dirname(requirement['import']))
                if not os.path.isdir(dest_dir):
                    os.makedirs(dest_dir)

                copy(os.path.join(base_dir, requirement['from'], requirement['import']),
                         dest_dir)
            else:
                raise ValueError('Unknown requirement type: ' + requirement['type'])

    for dev in config['devices']:
        dev_path = os.path.join(base_dir, dev)
        dest_dir = os.path.join('tmp/middleware', os.path.dirname(dev))
        if not os.path.isdir(dest_dir):
                os.makedirs(dest_dir)

        copy(dev_path, dest_dir)
        cpp_filename = os.path.join(os.path.dirname(dev_path), 
                                    os.path.basename(dev_path).split('.')[0] + '.cpp')
        if os.path.exists(cpp_filename):
            copy(cpp_filename, dest_dir)

def main(argv):
    cmd = argv[0]

    tmp_dir = 'tmp'
    if not os.path.exists(tmp_dir):
        os.makedirs(tmp_dir)

    with open(argv[1]) as config_file:
        config = yaml.load(config_file)
        for inc in config.get('includes', []): # TODO add basedir to includes
            config.update(yaml.load(open(os.path.join(argv[2], inc))))

    if cmd == '--generate':
        devices, obj_files = generate(config["devices"], argv[3])
        render_makefile(obj_files)
        render_device_table(devices)

    elif cmd == '--devices':
        hpp_files = []
        cpp_files = []
        for dev in config['devices']:
            dev_path = os.path.join(argv[2], dev)
            hpp_files.append(dev_path)
            cpp_filename = os.path.join(os.path.dirname(dev_path), 
                                        os.path.basename(dev_path).split('.')[0] + '.cpp')
            if os.path.exists(cpp_filename):
                cpp_files.append(cpp_filename)

        with open('tmp/.devices', 'w') as f:
            f.write(' '.join(hpp_files))
            f.write(' ' + ' '.join(cpp_files))

    elif cmd == '--requirements':
        if 'requirements' in config:
            install_requirements(config, argv[2])

    elif cmd == '--cross-compile':
        with open('tmp/.cross-compile', 'w') as f:
            f.write(config['cross-compile'])

    elif cmd == '--server-name':
        with open('tmp/.server-name', 'w') as f:
            f.write(config['server-name'])

    elif cmd == '--midware-path':
        with open('tmp/.midware-path', 'w') as f:
            f.write(config['middleware-path'])

    elif cmd == '--arch-flags':
        with open('tmp/.arch-flags', 'w') as f:
            f.write('"-' + ' -'.join(config['arch_flags']) + '"')

    elif cmd == '--optim-flags':
        with open('tmp/.optim-flags', 'w') as f:
            if not config['debug']['status']:
                f.write('"-' + ' -'.join(config['optimization_flags']) + '"')

    elif cmd == '--debug-flags':
        with open('tmp/.debug-flags', 'w') as f:
            if config['debug']['status']:
                f.write('"-' + ' -'.join(config['debug']['flags']) + '"')

    elif cmd == '--defines':
        with open('tmp/.defines', 'w') as f:
            f.write('"-D' + ' -D'.join(config['defines']) + '"')

    else:
        raise ValueError('Unknown command')

if __name__ == "__main__":
        main(sys.argv[1:])
