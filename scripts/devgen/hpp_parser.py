import CppHeaderParser

CSTR_TYPES = ["char *", "char*", "const char *", "const char*"]

def ERROR(line, message):
    raise ValueError("Error line " + str(line) + ": " + message)

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
    _set_iotype(operation, method['rtnType'])
    operation['prototype'] = _get_operation_prototype(method)
    operation['flags'] = ''
    return operation

def _get_operation_prototype(method):
    prototype = {}
    prototype['ret_type'] = method['rtnType']
    prototype['name'] = method['name']
    prototype['params'] = []

    for param in method['parameters']:
        prototype['params'].append({
          'name': str(param['name']),
          'type': param['type']
        })
    return prototype

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
        if method['name'] in [s + _class['name'] for s in ['','~']]:
            continue

        operation = _get_operation(method)
        if operation is not None:
            device['operations'].append(operation)
    return device
