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

        lines.append('    static_assert(req_buff_size <= cmd.buffer.size(), "Buffer size too small");\n\n');
        lines.append('    if (req_buff_size != cmd.payload_size) {\n')
        lines.append('        kserver->syslog.print<SysLog::ERROR>(\"[' + device.name + ' - ' + operation['name'] + '] Invalid payload size. Expected %zu bytes. Received %zu bytes.\\n\", req_buff_size, cmd.payload_size);\n')
        lines.append('        return -1;\n')
        lines.append('    }\n\n')

    lines.append('    constexpr size_t position0 = 0;\n')
    pos_cnt = 0
    before_vector = True

    for idx, pack in enumerate(packs):
        if pack['family'] == 'scalar':
            if before_vector:
                lines.append('    auto args_tuple' + str(idx) + ' = deserialize<position' + str(pos_cnt) + ', cmd.buffer.size(), ')
                print_type_list_pack(lines, pack)
                lines.append('>(cmd.buffer);\n')

                if idx < len(packs) - 1:
                    lines.append('\n    constexpr size_t position' + str(pos_cnt + 1) + ' = position' + str(pos_cnt)
                                  + ' + required_buffer_size<')
                    pos_cnt += 1
                    print_type_list_pack(lines, pack)
                    lines.append('>();\n')
            else: # After vector need to reload a buffer
                lines.append('    Buffer<required_buffer_size<')
                print_type_list_pack(lines, pack)
                lines.append('>()> buff' + str(idx) + ';\n')
                lines.append('    if (LOAD_BUFFER(buff' + str(idx) + ') < 0) {\n')
                lines.append('        kserver->syslog.print<SysLog::ERROR>(\"[' + device.name + ' - ' + operation['name'] + '] Load buffer failed.\\n");\n')
                lines.append('        return -1;\n')
                lines.append('    }\n')

                lines.append('\n    auto args_tuple' + str(idx) + ' = deserialize<0, buff' + str(idx) + '.size(), ')
                print_type_list_pack(lines, pack)
                lines.append('>(buff' + str(idx) + ');\n')

            for i, arg in enumerate(pack['args']):
                lines.append('    args.' + arg["name"] + ' = ' + 'std::get<' + str(i) + '>(args_tuple' + str(idx) + ');\n');
        elif pack['family'] == 'array':
            array_params = get_std_array_params(pack['args']['type'])

            if before_vector:
                lines.append('    args.' + pack['args']['name'] + ' = extract_array<position' + str(pos_cnt)
                              + ', ' + array_params['T'] + ', ' + array_params['N'] + '>(cmd.buffer.data);\n')

                if idx < len(packs) - 1:
                    lines.append('\n    constexpr size_t position' + str(pos_cnt + 1) + ' = position' + str(pos_cnt)
                                  + ' + size_of<' + array_params['T'] + ', ' + array_params['N'] + '>;\n')
                    pos_cnt += 1
            else: # After vector need to reload a buffer
                lines.append('\n    Buffer<size_of<' + array_params['T'] + ', ' + array_params['N'] + '>> buff' + str(idx) + ';\n')

                lines.append('    if (LOAD_BUFFER(buff' + str(idx) + ') < 0) {\n')
                lines.append('        kserver->syslog.print<SysLog::ERROR>(\"[' + device.name + ' - ' + operation['name'] + '] Load buffer failed.\\n");\n')
                lines.append('        return -1;\n')
                lines.append('    }\n')

                lines.append('\n    args.' + pack['args']['name'] + ' = extract_array<0, '
                                + array_params['T'] + ', ' + array_params['N'] + '>(buff' + str(idx) + '.data);\n')
        elif pack['family'] == 'vector':
            before_vector = False

            lines.append('    uint64_t length' + str(idx) + ' = std::get<0>(deserialize<position' + str(pos_cnt) + ', cmd.buffer.size(), uint64_t>(cmd.buffer));\n\n')
            lines.append('    if (RCV_VECTOR(args.' + pack['args']['name'] + ', length' + str(idx) + ') < 0) {\n')
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
        and separate them from the arrays '''
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
