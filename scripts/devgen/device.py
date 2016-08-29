# Loads and generates devices descriptions. Generates source code.
#
# (c) Koheron

import yaml
import os
import jinja2

import autogen_implementation
from middleware_handler import MiddlewareHandler

def GetClassName(dev_name):
    return 'KS_' + dev_name.capitalize()

class Device:
    def __init__(self, path, midware_path):
        print 'Parsing and analysing ' + path + '...'
        mid_handler = MiddlewareHandler(os.path.join(midware_path, path))
        self.header_path = os.path.dirname(path)
        self._data = mid_handler.get_device_data()
        self.fragments = mid_handler.get_fragments()

        self.operations = self._data['operations']
        self.name = self._data['name']
        self.raw_name = self._data['raw_name']
        self.class_name = GetClassName(self.name)
        self.objects = self._data['objects']
        self.includes = self._data['includes']

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
        with open(header_filename, 'w') as output:
            output.write(template.render(device=self))
