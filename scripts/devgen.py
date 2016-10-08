# (c) Koheron

import os
import re
import CppHeaderParser
import jinja2
import json

# -----------------------------------------------------------------------------------------
# Code generation
# -----------------------------------------------------------------------------------------

class Device:
    def __init__(self, path, base_dir):
        print 'Parsing and analysing ' + path + '...'
        dev = parse_header(os.path.join(base_dir, path))[0]
        self.header_path = os.path.dirname(path)
        self.calls = cmd_calls(dev)

        self.path = path
        self.operations = dev['operations']
        self.tag = dev['tag']
        self.name = dev['name']
        self.class_name = 'KS_' + self.tag.capitalize()
        self.objects = dev['objects']
        self.includes = dev['includes']
        self.id = None

def generate(devices_list, base_dir, build_dir):
    print devices_list
    devices = [] # List of generated devices
    obj_files = []  # Object file names
    dev_id = 2
    for path in devices_list or []:
        if path.endswith('.hpp') or path.endswith('.h'):
            device = Device(path, base_dir)
            device.id = dev_id
            dev_id +=1
            print('Generating ' + device.name + '...')

            template = get_renderer().get_template(os.path.join('scripts/templates', 'ks_device.hpp'))
            with open(os.path.join(build_dir, 'ks_' + device.tag.lower() + '.hpp'), 'w') as output:
                output.write(template.render(device=device))

            template = get_renderer().get_template(os.path.join('scripts/templates', 'ks_device.cpp'))
            with open(os.path.join(build_dir, 'ks_' + device.tag.lower() + '.cpp'), 'w') as output:
                output.write(template.render(device=device))

            devices.append(device)
    render_device_table(devices, build_dir)

# Number of operation in the KServer device
KSERVER_OP_NUM = 7

def get_max_op_num(devices):
    ''' Return the maximum number of operations '''
    def device_length(device):
        return max(len(device.operations), KSERVER_OP_NUM)
    return max(device_length(d) for d in devices)

def get_json(devices):
    data = [{
        'class': 'KServer',
        'id': 1,
        'functions': [
            {'name': 'get_version', 'id': 0, 'args': [], 'ret_type': 'const char *'},
            {'name': 'get_cmds', 'id': 1, 'args': [], 'ret_type': 'std::string'},
            {'name': 'get_stats', 'id': 2, 'args': [], 'ret_type': 'const char *'},
            {'name': 'get_dev_status', 'id': 3, 'args': [], 'ret_type': 'void'},
            {'name': 'get_running_sessions', 'id': 4, 'args': [], 'ret_type': 'const char *'},
            {'name': 'subscribe_pubsub', 'id': 5, 'args': [{'name': 'channel', 'type': 'uint32_t'}], 'ret_type': 'void'},
            {'name': 'pubsub_ping', 'id': 6, 'args': [], 'ret_type': 'void'}
        ]
    }]

    for device in devices:
        data.append({
            'class': device.name,
            'id': device.id,
            'functions': [{'name': op['name'], 'id': op['id'], 'ret_type': format_ret_type(device.name, op), 'args': op.get('args_client',[])} for op in device.operations]
        })

    return json.dumps(data, separators=(',', ':')).replace('"', '\\"').replace('\\\\','')

def get_renderer():
    renderer = jinja2.Environment(
      block_start_string = '{%',
      block_end_string = '%}',
      variable_start_string = '{{',
      variable_end_string = '}}',
      loader = jinja2.FileSystemLoader(os.path.abspath('.'))
    )

    def list_operations(device, max_op_num):
        list_ = map(lambda x: x['name'], device.operations)
        list_ = ['"%s"' % element for element in list_]
        empty_ops = ['""'] * (max_op_num - len(list_))
        return ','.join(list_ + empty_ops)

    def get_fragment(operation, device):
        return device.calls[operation['tag']]

    def get_parser(operation, device):
        return parser_generator(device, operation)

    renderer.filters['list_operations'] = list_operations
    renderer.filters['get_fragment'] = get_fragment
    renderer.filters['get_parser'] = get_parser
    return renderer

def fill_template(devices, template_filename, output_filename):
    template = get_renderer().get_template(os.path.join('scripts/templates', template_filename))
    with open(output_filename, 'w') as output:
        output.write(template.render(devices=devices))

def render_device_table(devices, build_dir):
    print('Generate device table')
    template = get_renderer().get_template(os.path.join('scripts/templates', 'devices_table.hpp'))
    with open(os.path.join(build_dir, 'devices_table.hpp'), 'w') as output:
        output.write(template.render(devices=devices,
                                     max_op_num=get_max_op_num(devices),
                                     json=get_json(devices)))

    output_filename = os.path.join(build_dir, 'devices.hpp')
    fill_template(devices, 'devices.hpp', output_filename)

# -----------------------------------------------------------------------------
# Parse device C++ header
# -----------------------------------------------------------------------------

def parse_header(hppfile):
    cpp_header = CppHeaderParser.CppHeader(hppfile)
    devices = []
    for classname in cpp_header.classes:
        devices.append(parse_header_device(cpp_header.classes[classname], hppfile))
    return devices

def parse_header_device(_class, hppfile):
    device = {}
    device['name'] = _class['name']
    device['tag'] = '_'.join(re.findall('[A-Z][^A-Z]*', device['name'])).upper()
    device['includes'] = [hppfile]
    device['objects'] = [{
      'type': str(_class['name']),
      'name': '__' + _class['name']
    }]

    device['operations'] = []
    op_id = 0
    for method in _class['methods']['public']:
        # We eliminate constructor and destructor
        if not (method['name'] in [s + _class['name'] for s in ['','~']]):
            device['operations'].append(parse_header_operation(device['name'], method))
            device['operations'][-1]['id'] = op_id
            op_id += 1
    return device

def parse_header_operation(devname, method):
    operation = {}
    operation['tag'] = method['name'].upper()
    operation['name'] = method['name']
    operation['ret_type'] = method['rtnType']
    check_type(operation['ret_type'], devname, operation['name'])

    operation['io_type'] = {}
    if operation['ret_type'] == 'void':
        operation['io_type'] = 'WRITE'
    elif operation['ret_type'] in ["char *", "char*", "const char *", "const char*"]:
        operation["io_type"] = 'READ_CSTR'
    else:
        operation["io_type"] = 'READ'

    if len(method['parameters']) > 0:
        operation['arguments'] = [] # Use for code generation
        operation['args_client'] = [] # Send to client
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

            check_type(arg['type'], devname, operation['name'])
            operation['arguments'].append(arg)
            operation['args_client'].append({'name': arg['name'], 'type': format_type(arg['type'])})
    return operation

# The following integers are forbiden since they are plateform
# dependent and thus not compatible with network use.
FORBIDDEN_INTS = ['short', 'int', 'unsigned', 'long', 'unsigned short', 'short unsigned',
                  'unsigned int', 'int unsigned', 'unsigned long', 'long unsigned',
                  'long long', 'unsigned long long', 'long long unsigned']

def check_type(_type, devname, opname):
    if _type in FORBIDDEN_INTS:
        raise ValueError('[' + devname + '::' + opname + '] Invalid type "' + _type + '": Only integers with exact width (e.g. uint32_t) are supported (http://en.cppreference.com/w/cpp/header/cstdint).')

def format_type(_type):
    if is_std_array(_type):
        templates = _type.split('<')[1].split('>')[0].split(',')
        return 'std::array<' + templates[0] + ', " << ' +  templates[1] + ' << ">'
    else:
        return _type

def format_ret_type(classname, operation):
    if operation['ret_type'] in ['auto', 'auto&', 'auto &']:
        decl_arg_list = []
        for arg in operation.get('arguments', []):
            decl_arg_list.append('std::declval<' + arg['type'] + '>()')
        return '" << get_type_str<decltype(std::declval<' + classname + '>().' + operation['name'] + '(' + ' ,'.join(decl_arg_list) + '))>() << "'
    else:
        return operation['ret_type']


# -----------------------------------------------------------------------------
# Generate command call and send
# -----------------------------------------------------------------------------

def cmd_calls(device):
    calls = {}
    for op in device['operations']:
        calls[op['tag']] = generate_call(device, op)
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

# -----------------------------------------------------------
# Parse command arguments
# -----------------------------------------------------------

def parser_generator(device, operation): 
    lines = []   
    if operation.get('arguments') is None:
        return ''

    packs, has_vector = build_args_packs(lines, operation)

    if not has_vector:
        print_req_buff_size(lines, packs)

        lines.append('    static_assert(req_buff_size <= cmd.payload.size(), "Buffer size too small");\n\n');
        lines.append('    if (req_buff_size != cmd.payload_size) {\n')
        lines.append('        kserver->syslog.print<SysLog::ERROR>(\"[' + device.name + ' - ' + operation['name'] + '] Invalid payload size. Expected %zu bytes. Received %zu bytes.\\n\", req_buff_size, cmd.payload_size);\n')
        lines.append('        return -1;\n')
        lines.append('    }\n\n')

    before_vector = True

    for idx, pack in enumerate(packs):
        if pack['family'] == 'scalar':
            if before_vector:
                lines.append('    auto args_tuple' + str(idx) + ' = cmd.payload.deserialize<')
                print_type_list_pack(lines, pack)
                lines.append('>();\n')

                for i, arg in enumerate(pack['args']):
                    lines.append('    args.' + arg["name"] + ' = ' + 'std::get<' + str(i) + '>(args_tuple' + str(idx) + ');\n');
            else: # After vector need to reload a buffer
                lines.append('\n    auto args_tuple' + str(idx)  + ' = DESERIALIZE<')
                print_type_list_pack(lines, pack)
                lines.append('>(cmd);\n')
                lines.append('    if (std::get<0>(args_tuple' + str(idx)  + ') < 0) {\n')
                lines.append('        kserver->syslog.print<SysLog::ERROR>(\"[' + device.name + ' - ' + operation['name'] + '] Failed to deserialize buffer.\\n");\n')
                lines.append('        return -1;\n')
                lines.append('    }\n')

                for i, arg in enumerate(pack['args']):
                    lines.append('    args.' + arg["name"] + ' = ' + 'std::get<' + str(i + 1) + '>(args_tuple' + str(idx) + ');\n');

        elif pack['family'] == 'array':
            array_params = get_std_array_params(pack['args']['type'])

            if before_vector:
                lines.append('    args.' + pack['args']['name'] + ' = cmd.payload.extract_array<' + array_params['T'] + ', ' + array_params['N'] + '>();\n')
            else: # After vector need to reload a buffer
                lines.append('\n    auto tup' + str(idx)  + ' = EXTRACT_ARRAY<' + array_params['T'] + ', ' + array_params['N'] + '>(cmd);\n')
                lines.append('    if (std::get<0>(tup' + str(idx)  + ') < 0) {\n')
                lines.append('        kserver->syslog.print<SysLog::ERROR>(\"[' + device.name + ' - ' + operation['name'] + '] Failed to extract array.\\n");\n')
                lines.append('        return -1;\n')
                lines.append('    }\n')

                lines.append('\n    args.' + pack['args']['name'] + ' = std::get<1>(tup' + str(idx)  + ');\n')

        elif pack['family'] == 'vector':
            before_vector = False

            lines.append('    uint64_t length' + str(idx) + ' = std::get<0>(cmd.payload.deserialize<uint64_t>());\n\n')
            lines.append('    if (RCV_VECTOR(args.' + pack['args']['name'] + ', length' + str(idx) + ', cmd) < 0) {\n')
            lines.append('        kserver->syslog.print<SysLog::ERROR>(\"[' + device.name + ' - ' + operation['name'] + '] Failed to receive vector.\\n");\n')
            lines.append('        return -1;\n')
            lines.append('    }\n\n')
        else:
            raise ValueError('Unknown argument family')
    return ''.join(lines)

def print_req_buff_size(lines, packs):
    lines.append('    constexpr size_t req_buff_size = ');

    for idx, pack in enumerate(packs):
        if pack['family'] == 'scalar':
            if idx == 0:
                lines.append('required_buffer_size<')
            else:
                lines.append('                                     + required_buffer_size<')
            print_type_list_pack(lines, pack)
            if idx < len(packs) - 1:
                lines.append('>()\n')
            else:
                lines.append('>();\n')
        elif pack['family'] == 'array':
            array_params = get_std_array_params(pack['args']['type'])
            if idx == 0:
                lines.append('size_of<')
            else:
                lines.append('                                     + size_of<')
            lines.append(array_params['T'] + ', ' + array_params['N'] + '>')
            if idx < len(packs) - 1:
                lines.append('\n')
            else:
                lines.append(';\n')
    lines.append('\n')

def print_type_list_pack(lines, pack):
    for idx, arg in enumerate(pack['args']):
        if idx < len(pack['args']) - 1:   
            lines.append(arg['type'] + ', ')
        else:
            lines.append(arg['type'])

def build_args_packs(lines, operation):
    ''' Packs the adjacent scalars together for deserialization
        and separate them from the arrays and vectors'''
    packs = []
    args_list = []
    has_vector = False
    for idx, arg in enumerate(operation["arguments"]):
        if is_std_array(arg['type']):
            if len(args_list) > 0:
                packs.append({'family': 'scalar', 'args': args_list})
                args_list = []
            packs.append({'family': 'array', 'args': arg})
        elif is_std_vector(arg['type']):
            has_vector = True
            if len(args_list) > 0:
                packs.append({'family': 'scalar', 'args': args_list})
                args_list = []
            packs.append({'family': 'vector', 'args': arg})
        else:
            args_list.append(arg)
    if len(args_list) > 0:
        packs.append({'family': 'scalar', 'args': args_list})
    return packs, has_vector

def is_std_array(arg_type):
    return arg_type.split('<')[0].strip() == 'std::array'

def is_std_vector(arg_type):
    return arg_type.split('<')[0].strip() == 'std::vector'

def get_std_array_params(arg_type):
    templates = arg_type.split('<')[1].split('>')[0].split(',')
    return {
      'T': templates[0].strip(),
      'N': templates[1].strip()
    }
