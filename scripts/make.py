#! /usr/bin/python

# (c) Koheron

import os
import sys
import yaml
import jinja2
import time
import subprocess
import json
from distutils.dir_util import copy_tree
from shutil import copy

from devgen import Device, parser_generator

# -----------------------------------------------------------------------------------------
# Code generation
# -----------------------------------------------------------------------------------------

# Number of operation in the KServer device
KSERVER_OP_NUM = 7

def get_max_op_num(devices):
    ''' Return the maximum number of operations '''
    def device_length(device):
        return max(len(device.operations), KSERVER_OP_NUM)
    return max(device_length(d) for d in devices)

def get_json(devices):
    data = []

    data.append({
        'name': 'NO_DEVICE',
        'operations': []
    })

    data.append({
        'name': 'KServer',
        'operations': [
            'get_version', 'get_cmds', 'get_stats', 'get_dev_status', 'get_running_sessions', 'subscribe_broadcast', 'broadcast_ping'
        ]
    })

    for device in devices:
        data.append({
            'name': device.raw_name,
            'operations': [op['raw_name'] for op in device.operations]
        })
    return json.dumps(data, separators=(',', ':')).replace('"', '\\"')


def get_renderer():
    renderer = jinja2.Environment(
      block_start_string = '{%',
      block_end_string = '%}',
      variable_start_string = '{{',
      variable_end_string = '}}',
      loader = jinja2.FileSystemLoader(os.path.abspath('.'))
    )
    def list_operations(device, max_op_num):
        list_ = map(lambda x: x['raw_name'], device.operations)
        list_ = ['"%s"' % element for element in list_]
        empty_ops = ['""'] * (max_op_num - len(list_))
        return ','.join(list_ + empty_ops)

    def get_fragment(operation, device):
        string = ""
        for frag in device.fragments:
            if operation['name'] == frag['name']:
                for line in frag['fragment']:
                    string += line
        return string

    def get_parser(operation, device):
        return parser_generator(device, operation)

    renderer.filters['list_operations'] = list_operations
    renderer.filters['get_fragment'] = get_fragment
    renderer.filters['get_parser'] = get_parser

    return renderer

def fill_template(devices, template_filename, output_filename):
    template = get_renderer().get_template(os.path.join('scripts/templates', template_filename))
    with open(output_filename, 'w') as output:
        output.write(template.render(devices=devices))

def render_device_table(devices):
    print('Generate device table')
    template = get_renderer().get_template(os.path.join('scripts/templates', 'devices_table.hpp'))
    with open('tmp/devices_table.hpp', 'w') as output:
        output.write(template.render(devices=devices,
                                     max_op_num=get_max_op_num(devices),
                                     json=get_json(devices)))

    output_filename = os.path.join('tmp', 'devices.hpp')
    fill_template(devices, 'devices.hpp', output_filename)

def generate(devices_list, midware_path):
    devices = [] # List of generated devices
    obj_files = []  # Object file names
    for path in devices_list:
        if path.endswith('.hpp') or path.endswith('.h'):
            device = Device(path, midware_path)
            print('Generating ' + device.name + '...')

            template = get_renderer().get_template(os.path.join('scripts/templates', 'ks_device.hpp'))
            with open(os.path.join(midware_path, os.path.dirname(path), 'ks_' + device.name.lower() + '.hpp'), 'w') as output:
                output.write(template.render(device=device))

            template = get_renderer().get_template(os.path.join('scripts/templates', 'ks_device.cpp'))
            with open(os.path.join(midware_path, os.path.dirname(path), 'ks_' + device.name.lower() + '.cpp'), 'w') as output:
                output.write(template.render(device=device))

            devices.append(device)
    return devices

# -----------------------------------------------------------------------------------------

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
        for dev in get_devices(config):
            dev_path = os.path.join(base_dir, dev)
            dest_dir = os.path.join('tmp/middleware', os.path.dirname(dev))
            if not os.path.isdir(dest_dir):
                    os.makedirs(dest_dir)

            copy(dev_path, dest_dir)
            cpp_filename = os.path.join(os.path.dirname(dev_path), 
                                        os.path.basename(dev_path).split('.')[0] + '.cpp')
            if os.path.exists(cpp_filename):
                copy(cpp_filename, dest_dir)

def get_devices(config):
    return config.get('devices')

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
        devices = generate(get_devices(config), argv[3])
        render_device_table(devices)

    elif cmd == '--devices':
        hpp_files = []
        cpp_files = []
        for dev in get_devices(config):
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
