#! /usr/bin/python

# (c) Koheron

import os
import sys
import yaml
import time
import subprocess
from distutils.dir_util import copy_tree
from shutil import copy

from devgen import generate

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

    if 'copy_devices_to_middleware' in config and config['copy_devices_to_middleware']:
        for device in get_devices(config):
            dev_path = os.path.join(base_dir, device)
            dest_dir = os.path.join('tmp/middleware', os.path.dirname(device))
            if not os.path.isdir(dest_dir):
                    os.makedirs(dest_dir)

            copy(dev_path, dest_dir)
            cpp_filename = os.path.join(os.path.dirname(dev_path), 
                                        os.path.basename(dev_path).split('.')[0] + '.cpp')
            if os.path.exists(cpp_filename):
                copy(cpp_filename, dest_dir)
                
def get_devices(config):
    if 'devices' in config:
        return config['devices']
    elif 'drivers' in config:
        return config['drivers']

def main(argv):
    cmd = argv[0]

    tmp_dir = 'tmp'
    if not os.path.exists(tmp_dir):
        os.makedirs(tmp_dir)

    with open(argv[1]) as config_file:
        config = yaml.load(config_file)
    for include_filename in config.get('includes', []):
        with open(os.path.join(argv[2], include_filename)) as include_file:
            for key, value in yaml.load(include_file).iteritems():
                if key in config:
                    config[key].extend(value)
                else:
                    config[key] = value

    if cmd == '--generate':
        generate(get_devices(config), argv[3])

    elif cmd == '--devices':
        hpp_files = []
        cpp_files = []
        for device in get_devices(config):
            dev_path = os.path.join(argv[2], device)
            hpp_files.append(dev_path)
            cpp_filename = os.path.join(os.path.dirname(dev_path), 
                                        os.path.basename(dev_path).split('.')[0] + '.cpp')
            if os.path.exists(cpp_filename):
                cpp_files.append(cpp_filename)

        with open('tmp/.devices', 'w') as f:
            f.write(' '.join(hpp_files))
            f.write(' ' + ' '.join(cpp_files))

    elif cmd == '--requirements':
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
            f.write('"-' + ' -'.join(config['optimization_flags']) + '"')

    elif cmd == '--debug-flags':
        with open('tmp/.debug-flags', 'w') as f:
            if config['debug']['status']:
                f.write('"-' + ' -'.join(config['debug']['flags']) + '"')

    elif cmd == '--defines':
        with open('tmp/.defines', 'w') as f:
            f.write('"-D' + ' -D'.join(config['defines']) + ' -DSHA=' + argv[3] + '"')

    else:
        raise ValueError('Unknown command')

if __name__ == "__main__":
        main(sys.argv[1:])
