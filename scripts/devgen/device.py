# (c) Koheron

import os
from middleware_handler import parse_header, FragmentsGenerator

class Device:
    def __init__(self, path, midware_path):
        print 'Parsing and analysing ' + path + '...'
        dev = parse_header(os.path.join(midware_path, path))[0]
        self.header_path = os.path.dirname(path)
        self.fragments = FragmentsGenerator(dev).get_fragments()

        self.operations = dev['operations']
        self.tag = dev['tag']
        self.name = dev['name']
        self.class_name = 'KS_' + self.tag.capitalize()
        self.objects = dev['objects']
        self.includes = dev['includes']
