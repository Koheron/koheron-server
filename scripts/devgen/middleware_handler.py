# Generates a device description dictionnary and code fragments 
# from a tagged C++ header file
#
# (c) Koheron

import os
import time
import yaml
import string
import re
import CppHeaderParser

CSTR_TYPES = ["char *", "char*", "const char *", "const char*"]

def parse_header(hpp_filename):
    try:
        cpp_header = CppHeaderParser.CppHeader(hpp_filename)
    except CppHeaderParser.CppParseError as e:
        print(e)

    devices = []
    for classname in cpp_header.classes:
        devices.append(_get_device(cpp_header.classes[classname]))
    return devices

def get_operation(method):
    operation = {}
    operation['tag'] = method['name'].upper()
    operation['name'] = method['name']
    operation['ret_type'] = method['rtnType']

    operation['io_type'] = {}
    if operation['ret_type'] == 'void':
        operation['io_type'] = {'value': 'WRITE', 'remaining': ''}
    elif operation['ret_type'] in CSTR_TYPES:
        operation["io_type"] = {'value': 'READ_CSTR', 'remaining': ''}
    else:
        operation["io_type"] = {'value': 'READ', 'remaining': ''}

    if len(method['parameters']) > 0:
        operation['arguments'] = []
        for param in method['parameters']:
            arg = {}
            arg['name'] = str(param['name'])
            arg['type'] = param['type'].strip()
            if arg['type'][-1:] == '&': # Argument passed by reference
                arg['by_reference'] = True
                arg['type'] = arg['type'][:-2].strip()

            if arg['type'][:5] == 'const':# Argument is const
                arg['is_const'] = True
                arg['type'] = arg['type'][5:].strip()
            operation['arguments'].append(arg)
    return operation

def _get_device(_class):
    device = {}
    device['objects'] = [{
      'name': _class['name'].lower(),
      'type': str(_class['name'])
    }]
    device['name'] = _class['name']

    device['operations'] = []
    for method in _class['methods']['public']:
        # We eliminate constructor and destructor
        if not (method['name'] in [s + _class['name'] for s in ['','~']]):
            device['operations'].append(get_operation(method))
    return device

class MiddlewareHppParser:
    def __init__(self, hppfile):
        devices = parse_header(hppfile)
        self.raw_dev_data = devices[0]
        self.raw_dev_data["includes"] = [os.path.basename(hppfile)];
        self.device = self._get_device()

    def _get_device(self):
        device = {}
        device['operations'] = self.raw_dev_data['operations']
        device['name'] = self.raw_dev_data['name']
        device['includes'] = self.raw_dev_data['includes']
        device['objects'] = [{
          'type': self.raw_dev_data['objects'][0]['type'],
          'name': '__' + self.raw_dev_data['name']
        }]
        return device

    def get_device_tag(self, name):
        return '_'.join(re.findall('[A-Z][^A-Z]*', name)).upper()

class FragmentsGenerator:
    def __init__(self, parser):
        self.parser = parser

    def get_fragments(self):
        fragments = []

        for op in self.parser.raw_dev_data['operations']:
            frag = {}
            frag['name'] = op['tag']
            frag['fragment'] = self.generate_fragment(op['tag'])
            fragments.append(frag)

        return fragments

    def generate_fragment(self, op_name):
        ''' Generate the fragment of an operation '''
        operation = self._get_operation_data(op_name)
        frag = []

        if operation['io_type']['value'] == 'WRITE':
            frag.append('    ' + self._build_func_call(operation) + ';\n')
            frag.append('    return 0;\n')
        elif operation['io_type']['value'] == 'READ':
            frag.append('    return SEND('+ self._build_func_call(operation) + ');\n')
        elif operation['io_type']['value'] == 'READ_CSTR':
            frag.append('    return SEND_CSTR(' + self._build_func_call(operation) + ');\n')
        return frag

    def _get_operation_data(self, op_name):
        for op in self.parser.device['operations']:
            if op['tag'] == op_name:
                return op
        raise ValueError("Unknown operation " + op_name)

    def _build_func_call(self, operation):
        call = 'THIS->' + self.parser.device['objects'][0]['name'] + '.' + operation['name'] + '('
        call += ', '.join('args.' + arg['name'] for arg in operation.get('arguments', []))
        return call + ')'
