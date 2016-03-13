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
    line_cnt = 0
    pragmas = []
    for line in fd.readlines():
        line_cnt = line_cnt + 1
        if '# pragma tcp-server' in line:
            pragmas.append({
              'line_number': line_cnt,
              'data': line.split("# pragma tcp-server")[1].strip()
            })
        elif '#pragma tcp-server' in line:
            pragmas.append({
              'line_number': line_cnt,
              'data': line.split("#pragma tcp-server")[1].strip()
            })
    fd.close()
    return pragmas

def _get_method_pragma(method, pragmas):
    if pragmas != None and len(pragmas) > 0:
        for pragma in pragmas:
            if pragma['line_number'] == method['line_number'] - 1:
                return pragma
    return None

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
    
    if pragma != None and pragma['data'] == 'exclude':
        return None
    
    operation = {}
    operation['prototype'] = _get_operation_prototype(method)
    _set_iotype(operation, method['rtnType'])
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

def _get_is_failed(_class, pragmas):
    for method in _class['methods']['public']:
        pragma = _get_method_pragma(method, pragmas)
        if pragma != None and pragma['data'] == 'is_failed':
            prototype = _get_operation_prototype(method)
            if prototype["ret_type"] != "bool":
                ERROR(pragma['line_number'], 'Failure indicator function must return a bool')
            if len(prototype['params']) > 0:
                ERROR(pragma['line_number'], 'Failure indicator function must not have argument')
            return prototype
    return None
        
def _get_device(_class, pragmas):
    device = {}
    device["objects"] = [{
      "name": _class['name'].lower(),
      "type": str(_class['name'])
    }]
    device["name"] = _class['name']
    
    is_failed_data = _get_is_failed(_class, pragmas)
    if is_failed_data != None:
        device['is_failed'] = is_failed_data
    
    device['operations'] = []
    for method in _class['methods']['public']:
        # We eliminate constructor and destructor
        if method['name'] == _class['name'] or method['name'] == '~' + _class['name']:
            continue
            
        if 'is_failed' in device and method['name'] == device['is_failed']['name']:
            continue
            
        operation = _get_operation(method, pragmas)
        if operation != None:
            device['operations'].append(operation)
    return device

