# Generate the implementation template for the device
#
# (c) Koheron

import os
import device as dev_utils

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

# -----------------------------------------------------------
# Autogenerate the parser
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
        lines.append('        kserver->syslog.print<SysLog::ERROR>(\"[' + device.name + ' - ' + operation['raw_name'] + '] Invalid payload size. Expected %zu bytes. Received %zu bytes.\\n\", req_buff_size, cmd.payload_size);\n')
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
                lines.append('        kserver->syslog.print<SysLog::ERROR>(\"[' + device.name + ' - ' + operation['raw_name'] + '] Failed to deserialize buffer.\\n");\n')
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
                lines.append('        kserver->syslog.print<SysLog::ERROR>(\"[' + device.name + ' - ' + operation['raw_name'] + '] Failed to extract array.\\n");\n')
                lines.append('        return -1;\n')
                lines.append('    }\n')

                lines.append('\n    args.' + pack['args']['name'] + ' = std::get<1>(tup' + str(idx)  + ');\n')

        elif pack['family'] == 'vector':
            before_vector = False

            lines.append('    uint64_t length' + str(idx) + ' = std::get<0>(cmd.payload.deserialize<uint64_t>());\n\n')
            lines.append('    if (RCV_VECTOR(args.' + pack['args']['name'] + ', length' + str(idx) + ', cmd) < 0) {\n')
            lines.append('        kserver->syslog.print<SysLog::ERROR>(\"[' + device.name + ' - ' + operation['raw_name'] + '] Failed to receive vector.\\n");\n')
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
