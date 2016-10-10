#! /usr/bin/python

# (c) Koheron

import os
import sys
import yaml
import json
import time
import subprocess
from distutils.dir_util import copy_tree
from shutil import copy

from devgen import generate
             
def get_devices(config):
    if 'devices' in config:
        return config['devices']
    elif 'drivers' in config:
        return config['drivers']

def main(argv):
    cmd = argv[0]

    tmp_dir = argv[3]
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

    if cmd == '--config':
        with open(os.path.join(tmp_dir, 'full_config.json'), 'w') as f:
            json.dump(config, f)

    elif cmd == '--generate':
        generate(get_devices(config), argv[2], tmp_dir)

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

        with open(os.path.join(tmp_dir, '.devices'), 'w') as f:
            f.write(' '.join(hpp_files))
            f.write(' ' + ' '.join(cpp_files))

    elif cmd == '--dependencies':
        with open(os.path.join(tmp_dir, '.dependencies'), 'w') as f:
            if 'dependencies' in config:
                f.write(' '.join((os.path.join(argv[2], dep) for dep in config['dependencies'])))

    elif cmd == '--debug-flags':
        with open(os.path.join(tmp_dir, '.debug-flags'), 'w') as f:
            if config['debug']['status']:
                f.write('-' + ' -'.join(config['debug']['flags']))

    else:
        raise ValueError('Unknown command')

if __name__ == "__main__":
        main(sys.argv[1:])
