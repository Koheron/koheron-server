# @file generator.py
#
# @brief Generate KServer sources
#
# @author Thomas Vanderbruggen <thomas@koheron.com>
# @date 23/12/2014
#
# (c) Koheron 2014-2015

import os
import yaml
import subprocess
import jinja2
import time
from shutil import rmtree, copyfile, copytree

from device import Device
import device_table

class Generator:
    def __init__(self, config_filename, src_dir='tmp/server'):
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
            
        if "host" in config:
            self.host = config["host"]
        else:
            self.host = None
            
        if "cross-compile" in config:
            self.cross_compile = config["cross-compile"]
        else:
            self.cross_compile = None
            
        if self.host == None and self.cross_compile == None:
            raise ValueError("Specify a target host or a cross-compilation toolchain")
                
        # Generate devices
        base_dir = os.path.dirname(config_filename)
        gen_devices = [] # List of generated devices
        self.obj_files = []  # Object file names
        
        for path in config["devices"]:       
            device = Device(path, base_dir)
            
            print "Generate " + device.name
            device.Generate(self.dev_dir)
            
            gen_devices.append(device)
            self.obj_files.append(os.path.join('devices', 
                                               device.class_name.lower()+'.o'))
         
        print "Generate device table"
        device_table.PrintDeviceTable(gen_devices, self.dev_dir)
        device_table.PrintDevicesHeader(gen_devices, self.dev_dir)
        
    def compile(self):
        """ Compile KServer
    
        Args:
            objects: name of the object files to be compiled
        """
        # Copy core cpp files and middleware in src_dir
        copytree('core', os.path.join(self.src_dir, 'core'))
        os.remove(os.path.join(self.src_dir, 'core/Makefile'))
        os.remove(os.path.join(self.src_dir, 'core/main.cpp'))
        
        copytree('middleware', os.path.join(self.src_dir, 'middleware'))
        os.remove(os.path.join(self.src_dir, 'middleware/README.md'))
        copyfile('core/main.cpp', os.path.join(self.src_dir, 'main.cpp'))

        # Generate Makefile
        print "Generate Makefile"
        self._makefile_gen('core/Makefile')
          
        # Call g++
        print "Compile ..."
        
        if self.host != None:
            subprocess.check_call("make -C tmp/server TARGET_HOST=" + self.host + " clean all", shell=True)
        
        if self.cross_compile != None:
            subprocess.check_call("make -C tmp/server CROSS_COMPILE=" + self.cross_compile + "- clean all", shell=True)
        
    def _makefile_gen(self, template_filename):
        """ Generate the template for KServer makefile

        Args:
            - template_filename: name to the Makefile template
        """
        makefile_renderer = jinja2.Environment(
          loader = jinja2.FileSystemLoader(os.path.abspath('.'))
        )

        template = makefile_renderer.get_template(template_filename)
        date = time.strftime("%c")
        makefile_filename = os.path.join(self.src_dir, 'Makefile')

        output = file(makefile_filename, 'w')
        output.write(template.render(date=date, objs_list=self.obj_files))
        output.close()
        
