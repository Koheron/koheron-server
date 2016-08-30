# (c) Koheron

import os
from middleware_handler import MiddlewareHandler

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
        self.class_name = 'KS_' + self.name.capitalize()
        self.objects = self._data['objects']
        self.includes = self._data['includes']
