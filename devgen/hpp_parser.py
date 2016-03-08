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

def _set_iotype(operation, _type):
    operation['io_type'] = {}
    if _type == 'void':
        operation['io_type'] = {'value': 'WRITE', 'remaining': ''}
    elif _type in CSTR_TYPES:
        operation["io_type"] = {'value': 'READ_CSTR', 'remaining': ''}
    else:
        operation["io_type"] = {'value': 'READ', 'remaining': ''}
    
def _get_operation(method):
    operation = {}
    operation['prototype'] = {}
    operation['prototype']['ret_type'] = method['rtnType']
    _set_iotype(operation, method['rtnType'])
    operation['prototype']['name'] = method['name']
    operation['prototype']['params'] = []
    
    for param in method['parameters']:
        operation['prototype']['params'].append({
          'name': str(param['name']),
          'type': param['type']
        })
    
    return operation
    
def _get_device(_class):
    device = {}
    device["objects"] = [{
      "name": _class['name'].lower(),
      "type": str(_class['name'])
    }]
    device["name"] = _class['name']
    device['operations'] = []
    for method in _class['methods']['public']:
        # We eliminate constructor and destructor
        if method['name'] != _class['name'] and method['name'] != '~' + _class['name']:
            device['operations'].append(_get_operation(method))
    return device

    

