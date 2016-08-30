# (c) Koheron

import os
from middleware_handler import MiddlewareHppParser, FragmentsGenerator

class Device:
    def __init__(self, path, midware_path):
        print 'Parsing and analysing ' + path + '...'
        parser = MiddlewareHppParser(os.path.join(midware_path, path))
        self.header_path = os.path.dirname(path)
        self.fragments = FragmentsGenerator(parser).get_fragments()

        self.operations = parser.device['operations']
        self.tag = parser.get_device_tag(parser.device['name'])
        self.name = parser.device['name']
        self.class_name = 'KS_' + self.tag.capitalize()
        self.objects = parser.device['objects']
        self.includes = parser.device['includes']
