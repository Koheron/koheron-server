# Generate Koheron server sources
#
# (c) Koheron

import os
import yaml
import jinja2
import time
from shutil import rmtree, copyfile, copytree

from device import Device
import device_table

class Generator:
    def __init__(self, config_filename, middleware_path, src_dir='tmp/server'):
        """ Generate the KServer sources
        
            Args:
              - config_filename: Configuration file
              - src_dir: Directory where the generated sources must be stored
        """
        # Set-up directories
        self.src_dir = src_dir
        self.dev_dir = os.path.join(self.src_dir, 'devices')
        
        if os.path.isdir(self.src_dir):
            rmtree(self.src_dir)

        os.makedirs(self.dev_dir)

        # Load configuration file
        with open(config_filename) as config_file:    
            config = yaml.load(config_file)

        # Copy core cpp files and middleware in src_dir
        copytree('core', os.path.join(self.src_dir, 'core'))
        os.remove(os.path.join(self.src_dir, 'core/Makefile'))
        os.remove(os.path.join(self.src_dir, 'core/main.cpp'))
        copytree(middleware_path, os.path.join(self.src_dir, 'middleware'))
        copyfile('core/main.cpp', os.path.join(self.src_dir, 'main.cpp'))

        # Generate devices
        base_dir = os.path.dirname(config_filename)
        devices = [] # List of generated devices
        self.obj_files = []  # Object file names

        for path in config["devices"]:       
            device = Device(path, base_dir)
            
            print "Generate " + device.name
            device.Generate(self.dev_dir)
            
            devices.append(device)
            self.obj_files.append(os.path.join('devices', 
                                               device.class_name.lower()+'.o'))

        print "Generate device table"
        device_table.PrintDeviceTable(devices, self.dev_dir)
        self.render_devices_header(devices)

    def render_devices_header(self, devices):
        template_filename = 'devgen/templates/devices.hpp'

        header_renderer = jinja2.Environment(
          loader = jinja2.FileSystemLoader(os.path.abspath('.'))
        )

        template = header_renderer.get_template(template_filename)
        header_filename = os.path.join(self.dev_dir, 'devices.hpp')
        output = file(header_filename, 'w')
        output.write(template.render(devices=devices))
        output.close()
