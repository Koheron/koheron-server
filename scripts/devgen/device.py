# (c) Koheron

import os
import re
import CppHeaderParser

class Device:
    def __init__(self, path, midware_path):
        print 'Parsing and analysing ' + path + '...'
        dev = parse_header(os.path.join(midware_path, path))[0]
        self.header_path = os.path.dirname(path)
        self.calls = cmd_calls(dev)

        self.operations = dev['operations']
        self.tag = dev['tag']
        self.name = dev['name']
        self.class_name = 'KS_' + self.tag.capitalize()
        self.objects = dev['objects']
        self.includes = dev['includes']

# -----------------------------------------------------------------------------
# Parse device C++ header
# -----------------------------------------------------------------------------

def parse_header(hppfile):
    try:
        cpp_header = CppHeaderParser.CppHeader(hppfile)
    except CppHeaderParser.CppParseError as e:
        print(e)
        raise RuntimeError('Error parsing header file ' + hppfile)

    devices = []
    for classname in cpp_header.classes:
        devices.append(parse_header_device(cpp_header.classes[classname], hppfile))
    return devices

def parse_header_device(_class, hppfile):
    device = {}
    device['name'] = _class['name']
    device['tag'] = '_'.join(re.findall('[A-Z][^A-Z]*', device['name'])).upper()
    device['includes'] = [os.path.basename(hppfile)]
    device['objects'] = [{
      'type': str(_class['name']),
      'name': '__' + _class['name']
    }]

    device['operations'] = []
    for method in _class['methods']['public']:
        # We eliminate constructor and destructor
        if not (method['name'] in [s + _class['name'] for s in ['','~']]):
            device['operations'].append(parse_header_operation(method))
    return device

def parse_header_operation(method):
    operation = {}
    operation['tag'] = method['name'].upper()
    operation['name'] = method['name']
    operation['ret_type'] = method['rtnType']

    operation['io_type'] = {}
    if operation['ret_type'] == 'void':
        operation['io_type'] = 'WRITE'
    elif operation['ret_type'] in ["char *", "char*", "const char *", "const char*"]:
        operation["io_type"] = 'READ_CSTR'
    else:
        operation["io_type"] = 'READ'

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

# -----------------------------------------------------------------------------
# Generate command call and send
# -----------------------------------------------------------------------------

def cmd_calls(device):
    calls = []
    for op in device['operations']:
        call = {}
        call['name'] = op['tag']
        call['lines'] = generate_call(device, op)
        calls.append(call)
    return calls

def generate_call(device, operation):
    lines = []
    if operation['io_type'] == 'WRITE':
        lines.append('    ' + build_func_call(device, operation) + ';\n')
        lines.append('    return 0;\n')
    elif operation['io_type'] == 'READ':
        lines.append('    return SEND(' + build_func_call(device, operation) + ');\n')
    elif operation['io_type'] == 'READ_CSTR':
        lines.append('    return SEND_CSTR(' + build_func_call(device, operation) + ');\n')
    return ''.join(lines)

def build_func_call(device, operation):
    call = 'THIS->' + device['objects'][0]['name'] + '.' + operation['name'] + '('
    call += ', '.join('args.' + arg['name'] for arg in operation.get('arguments', []))
    return call + ')'