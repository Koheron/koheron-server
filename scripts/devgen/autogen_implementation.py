# Generate the implementation template for the device
#
# (c) Koheron

import os
import device as dev_utils
import pprint

def Generate(device, directory):
    filename = os.path.join(directory, device.class_name.lower() + '.cpp')
    f = open(filename, 'w')
        
    try:      
        PrintFileHeader(f, os.path.basename(filename))
        
        f.write('#include "' + device.class_name.lower() + '.hpp' + '"\n\n')
        
        f.write('#include <core/commands.hpp>\n')
        f.write('#include <core/kserver.hpp>\n')
        f.write('#include <core/kserver_session.hpp>\n')
        
        f.write('namespace kserver {\n\n')
        
        f.write("#define THIS (static_cast<" + device.class_name + "*>(this))\n\n")
        
        for operation in device.operations:
            f.write('/////////////////////////////////////\n')
            f.write('// ' + operation["name"] + '\n\n')
            
            PrintParseArg(f, device, operation)
            PrintExecuteOp(f, device, operation)
        
        PrintIsFailed(f, device)
        PrintExecute(f, device)
           
        f.write('} // namespace kserver\n\n')
        
        f.close()
    except:
        f.close()
        os.remove(filename)
        raise
    
def PrintFileHeader(file_id, filename):
    file_id.write('/// ' + filename + '\n')
    file_id.write('///\n')
    file_id.write('/// Generated by devgen. \n')
    file_id.write('/// DO NOT EDIT. \n')
    file_id.write('///\n')
    file_id.write('/// (c) Koheron \n\n')
    
# -----------------------------------------------------------
# PrintParseArg:
# Autogenerate the parser
# -----------------------------------------------------------
    
def PrintParseArg(file_id, device, operation):
    file_id.write('template<>\n')
    file_id.write('template<>\n')
    
    file_id.write('int KDevice<' + device.class_name + ',' + device.name + '>::\n')
    file_id.write('        parse_arg<' + device.class_name + '::' \
                    + operation["name"] + '> (const Command& cmd,\n' )
    file_id.write('                KDevice<' + device.class_name + ',' \
                        + device.name + '>::\n')
    file_id.write('                Argument<' + device.class_name \
                    + '::' + operation["name"] + '>& args)\n' )
    file_id.write('{\n')
    
    try:
        PrintParserCore(file_id, device, operation)
    except TypeError:
        raise
        
    file_id.write('    return 0;\n')
    file_id.write('}\n\n')
    
def PrintParserCore(file_id, device, operation):    
    if GetTotalArgNum(operation) == 0:
        return

    file_id.write('    static_assert(required_buffer_size<')
    PrintTypeList(file_id, operation)
    file_id.write('>() < cmd.buffer.size(),\n                  "Buffer size too small");\n\n');

    file_id.write('    if (required_buffer_size<')
    PrintTypeList(file_id, operation)
    file_id.write('>() != cmd.payload_size) {\n')
    file_id.write("        kserver->syslog.print(SysLog::ERROR, \"Invalid payload size\\n\");\n")
    file_id.write("        return -1;\n")
    file_id.write("    }\n\n")

    file_id.write('    constexpr size_t position0 = 0;\n')
    pos_cnt = 0

    packs = build_args_packs(file_id, operation)

    for idx, pack in enumerate(packs):
        if pack['familly'] == 'scalar':
            file_id.write('    auto args_tuple' + str(idx) + ' = deserialize<position' + str(pos_cnt) + ', cmd.buffer.size(), ')
            print_type_list_pack(file_id, pack)
            file_id.write('>(cmd.buffer);\n')

            for i, arg in enumerate(pack['args']):
                file_id.write('    args.' + arg["name"] + ' = ' + 'std::get<' + str(i) + '>(args_tuple' + str(idx) + ');\n');

            if idx < len(packs) - 2:
                file_id.write('\n    constexpr size_t position' + str(pos_cnt + 1) + ' = position' + str(pos_cnt)
                              + ' + required_buffer_size<')
                pos_cnt += 1
                print_type_list_pack(file_id, pack)
                file_id.write('>();\n')

        elif pack['familly'] == 'array':
            array_params = get_std_array_params(pack['args']['type'])
            print array_params
            file_id.write('    args.' + pack['args']['name'] + ' = extract_array<position' + str(pos_cnt)
                          + ', ' + array_params['T'] + ', ' + array_params['N'] + '>(cmd.buffer.data);\n')

            if idx < len(packs) - 1:
                file_id.write('\n    constexpr size_t position' + str(pos_cnt + 1) + ' = position' + str(pos_cnt)
                              + ' + size_of<' + array_params['T'] + ', ' + array_params['N'] + '>;\n')
                pos_cnt += 1
        else:
            raise ValueError('Unknown argument familly')

def PrintTypeList(file_id, operation):
    for idx, arg in enumerate(operation["arguments"]):
        if idx < GetTotalArgNum(operation) - 1:   
            file_id.write(arg["type"] + ', ')
        else:
            file_id.write(arg["type"])

def print_type_list_pack(file_id, pack):
    for idx, arg in enumerate(pack['args']):
        if idx < len(pack['args']) - 1:   
            file_id.write(arg['type'] + ', ')
        else:
            file_id.write(arg['type'])

# Groups the adjacent scalars together for deserialization and identify the arrays
def build_args_packs(file_id, operation):
    packs = []
    args_list = []
    for idx, arg in enumerate(operation["arguments"]):
        if not is_std_array(arg['type']):
            args_list.append(arg)
        else: # std::array
            packs.append({'familly': 'scalar', 'args': args_list})
            args_list = []
            packs.append({'familly': 'array', 'args': arg})
    if len(args_list) > 0:
        packs.append({'familly': 'scalar', 'args': args_list})
    # print pprint.pprint(packs)
    return packs

def is_std_array(arg_type):
    return arg_type.split('<')[0].strip() == 'std::array'

def get_std_array_params(arg_type):
    templates = arg_type.split('<')[1].split('>')[0].split(',')
    return {
      'T': templates[0].strip(),
      'N': templates[1].strip()
    }
    
def GetTotalArgNum(operation):
    if not dev_utils.IsArgs(operation):
        return 0
            
    return len(operation["arguments"])
# -----------------------------------------------------------
# ExecuteOp
# -----------------------------------------------------------
    
def PrintExecuteOp(file_id, device, operation):
    file_id.write('template<>\n')
    file_id.write('template<>\n')

    file_id.write('int KDevice<' + device.class_name + ',' \
                                    + device.name + '>::\n')
    file_id.write('        execute_op<' + device.class_name + '::' \
                            + operation["name"] + '> \n' )

    file_id.write('        (const Argument<' + device.class_name + '::' \
                            + operation["name"] + '>& args, SessID sess_id)\n')

    file_id.write('{\n')

    # Load code fragments
    for frag in device.fragments:
        if operation["name"] == frag['name']:        
            for line in frag['fragment']:
                file_id.write(line)

    file_id.write('}\n\n')

def PrintIsFailed(file_id, device):
    file_id.write('template<>\n')
    file_id.write('bool KDevice<' + device.class_name + ',' \
                    + device.name + '>::is_failed(void)\n')
    file_id.write('{\n')

    for frag in device.fragments:
        if frag['name'] == "IS_FAILED":        
            for line in frag['fragment']:
                file_id.write(line)

    file_id.write('}\n\n')

def PrintExecute(file_id, device):
    file_id.write('template<>\n')
    file_id.write('int KDevice<' + device.class_name \
                                + ',' + device.name + '>::\n')
    file_id.write('        execute(const Command& cmd)\n' )
    file_id.write('{\n')

    file_id.write('#if KSERVER_HAS_THREADS\n')
    file_id.write('    std::lock_guard<std::mutex> lock(THIS->mutex);\n')
    file_id.write('#endif\n\n')

    file_id.write('    switch(cmd.operation) {\n')

    for operation in device.operations:
        file_id.write('      case ' + device.class_name + '::' \
                                + operation["name"] + ': {\n')
        file_id.write('        Argument<' + device.class_name + '::' \
                                + operation["name"] + '> args;\n\n')
        file_id.write('        if (parse_arg<' + device.class_name + '::' \
                                + operation["name"] + '>(cmd, args) < 0)\n')
        file_id.write('            return -1;\n\n')
        file_id.write('        return execute_op<' + device.class_name + '::' \
                                + operation["name"] + '>(args, cmd.sess_id);\n')                                             
        file_id.write('      }\n')

    file_id.write('      case ' + device.class_name + '::' \
                                + device.name.lower() + '_op_num:\n')
    file_id.write('      default:\n')
    file_id.write('          kserver->syslog.print(SysLog::ERROR, "' 
                            + device.class_name + ': Unknown operation\\n");\n')
    file_id.write('          return -1;\n')
    file_id.write('    }\n')
    file_id.write('}\n\n')
