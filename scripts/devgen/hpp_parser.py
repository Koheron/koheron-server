import CppHeaderParser

CSTR_TYPES = ["char *", "char*", "const char *", "const char*"]

def ERROR(line, message):
    raise ValueError("Error line " + str(line) + ": " + message)

def parse_header(hpp_filename):
    try:
        cpp_header = CppHeaderParser.CppHeader(hpp_filename)
    except CppHeaderParser.CppParseError as e:
        print(e)

    pragmas = _get_pragmas(hpp_filename)
    devices = []
    for classname in cpp_header.classes:
        devices.append(_get_device(cpp_header.classes[classname], pragmas))
    return devices

def _get_pragmas(hpp_filename):
    fd = open(hpp_filename, 'r')
    pragmas = []
    for cnt, line in enumerate(fd.readlines()):
        if '# pragma koheron-server' in line:
            pragmas.append({
              'line_number': cnt + 1,
              'data': line.split('# pragma koheron-server')[1].strip()
            })
        elif '#pragma koheron-server' in line:
            pragmas.append({
              'line_number': cnt + 1,
              'data': line.split('#pragma koheron-server')[1].strip()
            })
    fd.close()
    return pragmas

def _get_method_pragma(method, pragmas):
    line_number = method['line_number'] - 1
    return next((p for p in pragmas if p['line_number'] == line_number), None)

def _set_iotype(operation, _type):
    operation['io_type'] = {}
    if _type == 'void':
        operation['io_type'] = {'value': 'WRITE', 'remaining': ''}
    elif _type in CSTR_TYPES:
        operation["io_type"] = {'value': 'READ_CSTR', 'remaining': ''}
    else:
        operation["io_type"] = {'value': 'READ', 'remaining': ''}

def _get_operation(method, pragmas):
    pragma = _get_method_pragma(method, pragmas)
    operation = {}

    if pragma is not None:
        if pragma['data'] == 'exclude':
            return None
        elif pragma['data'].find('read_array') >= 0:
            remaining = pragma['data'].split('read_array')[1].strip()
            operation['io_type'] = {'value': 'READ_ARRAY', 'remaining': remaining}
        else:
            _set_iotype(operation, method['rtnType'])
    else:
        _set_iotype(operation, method['rtnType'])

    operation['prototype'] = _get_operation_prototype(method)
    operation['flags'] = ''
    return operation

def _get_write_array_params(remaining):
    tokens = remaining.split()
    if len(tokens) != 2:
        raise ValueError('Line ' + pragma['line_number' ] 
                         + ': write_array expects to arguments: pointer and length')

    array_params = {}
    if tokens[0].find('arg') >= 0:
        array_params['name'] = {}
        array_params['name']['src'] = 'param'
        array_params['name']['name'] = tokens[0].split('{')[1].split('}')[0].strip()
    else:
        raise ValueError('Line ' + pragma['line_number' ] + ': the pointer must be an argument')

    if tokens[1].find('arg') >= 0:
        array_params['length'] = {}
        array_params['length']['src'] = 'param'
        array_params['length']['length'] = tokens[1].split('{')[1].split('}')[0].strip()
    else:
        raise ValueError('Line ' + pragma['line_number' ] + ': the length must be an argument')

    return array_params

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

def _get_device(_class, pragmas):
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

        operation = _get_operation(method, pragmas)
        if operation is not None:
            device['operations'].append(operation)
    return device
