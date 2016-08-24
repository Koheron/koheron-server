# Loads and generates devices descriptions. Generates source code.
#
# (c) Koheron

import yaml
import os
import jinja2

import autogen_implementation
from middleware_handler import MiddlewareHandler

def GetClassName(dev_name):
    return 'KS_' + dev_name[0].upper() + dev_name[1:].lower()

def IsFlags(operation):
    ''' Test whether flags are defined for a given operation '''
    return operation.get('flags') is not None

def IsArgs(operation):
    ''' Test whether arguments are defined for a given operation '''
    return operation.get('arguments') is not None

def IsDefaultVal(arg):
    ''' Test whether a default value is defined for an argument '''
    return len(operation.get('default')) > 0

def IsReturn(operation):
    ''' Test whether a return description are defined for a given operation '''
    return operation.get('return') is not None

class Device:
    def __init__(self, path, midware_path):
        print 'Parsing and analysing ' + path + '...'
        mid_handler = MiddlewareHandler(os.path.join(midware_path, path))
        self.header_path = os.path.dirname(path)
        self._data = mid_handler.get_device_data()
        self.fragments = mid_handler.get_fragments()

        self.operations = Operations(self._data['operations'])
        self.name = self._data['name']
        self.raw_name = self._data['raw_name']
        self.class_name = GetClassName(self.name)
        self.objects = Objects(self._data['objects'])
        self.includes = Includes(self._data['includes'])

    def generate(self, directory):
        self._render_ks_device_header(directory)          # Generate KServer header file (hpp)
        autogen_implementation.Generate(self, directory)  # Generate KServer implementation (cpp)

    def _render_ks_device_header(self, directory):
        template_filename = 'scripts/templates/ks_device.hpp'

        header_renderer = jinja2.Environment(
          loader = jinja2.FileSystemLoader(os.path.abspath('.'))
        )

        template = header_renderer.get_template(template_filename)

        header_filename = os.path.join(directory, self.class_name.lower() + '.hpp')
        output = file(header_filename, 'w')
        output.write(template.render(device=self))
        output.close()

class Operations:
    def __init__(self, operations):
        self._operations = operations

    def __len__(self):
        return len(self._operations)

    def __getitem__(self, index):
        if index >= len(self):
            raise IndexError
        return self._operations[index]

    def is_valid_op(self, operation_name):
        ''' Test whether an operation is valid.'''
        return any(operation_name in op for op in self)

class Objects:
    ''' Objects from external API '''

    def __init__(self, objects):
        self._objects = [] is objects is None else objects

    def __len__(self):
        return len(self._objects)

    def __getitem__(self, index):
        if index >= len(self):
            raise IndexError
        return self._objects[index]

class Includes:
    ''' API files to include '''
    def __init__(self, includes):
        self._includes = [] if includes is None else includes

    def __len__(self):
        return len(self._includes)

    def __getitem__(self, index):
        if index >= len(self):
            raise IndexError
        return self._includes[index]
