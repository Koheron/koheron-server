# Device table generation
#
# (c) Koheron

import os
import yaml
import jinja2

#--------------------------------------------
# Generate device_table.hpp
#--------------------------------------------

# Number of operation in the KServer device
KSERVER_OP_NUM = 5

def PrintDeviceTable(devices, src_dir):
    devtab_filename = os.path.join(src_dir, 'devices_table.hpp')
    f = open(devtab_filename, 'w')
        
    PrintFileHeader(f, 'device_table.hpp')        
    f.write('#ifndef __DEVICES_TABLE_HPP__\n')
    f.write('#define __DEVICES_TABLE_HPP__\n\n')
    
    f.write('#include <array>\n\n')

    max_op_num = GetMaxOpNum(devices)      
    PrintDevTable(f, devices, max_op_num)
                               
    f.write('\n\n/// Maximum number of operations\n')
    f.write('#define MAX_OP_NUM ' + str(max_op_num) + '\n\n')
    
    PrintEnum(f, devices, max_op_num)
    PrintDevDescription(f, devices, max_op_num)
    
    f.write('#endif // __DEVICES_TABLE_HPP__\n')
    return
    
def GetMaxOpNum(devices):
    """
    Return the list of the compiled devices and the maximum number of operations
    """
    max_op_num = KSERVER_OP_NUM
        
    for device in devices:
        if len(device.operations) > max_op_num:
            max_op_num = len(device.operations)
            
    return max_op_num

def PrintFileHeader(file_id, filename):
    file_id.write('/// @file '+filename+'\n')
    file_id.write('///\n')
    file_id.write('/// Autogenerated by Kdevgen \n')
    file_id.write('/// (c) Koheron 2014-2015 \n\n')
    return
    
def PrintDevTable(file_id, devices, max_op_num):
    file_id.write("#define DEVICES_TABLE(ENTRY)")
    for device in devices:
        PrintDeviceTableEntry(file_id, device, max_op_num)
    return
    
def PrintDeviceTableEntry(file_id, device, max_op_num):
    file_id.write('    \\\n  ENTRY(' + device.name + ', ' + device.class_name + ', ' )

    op_num = len(device.operations)
    
    for idx, operation in enumerate(device.operations):
        if idx == op_num-1:
            break
        
        file_id.write('"' + operation["name"] + '", ')
    
    if op_num == max_op_num:
        file_id.write('"' +
            device.operations[op_num-1]["name"] + '")')
    else:
        file_id.write('"' +
            device.operations[op_num-1]["name"] + '", ')
        for i in range(op_num, max_op_num-1):
            file_id.write('"",')
            
        file_id.write('"")')
            
    return

def PrintEnum(file_id, devices, max_op_num):
    file_id.write('/// Devices #\n' )
    file_id.write('typedef enum {\n' )
    file_id.write('    NO_DEVICE,\n' )
    file_id.write('    KSERVER,\n' )
    
    for device in devices:
        file_id.write('    ' + device.name.upper() + ',\n' )
    
    file_id.write('    device_num\n' )
    file_id.write('} device_t;\n\n' )
    return
    
def PrintDevDescription(file_id, devices, max_op_num):
    file_id.write('/// String descriptions of the devices and their related operations\n' )
    file_id.write('static const std::array< std::array< std::string, MAX_OP_NUM+1 >, device_num >\n' )
    file_id.write('device_desc = {{\n' )

    file_id.write('  {{"NO_DEVICE", ')
    for i in range(0,max_op_num-1):
            file_id.write('"", ')
    file_id.write('""}},\n')
    
    if max_op_num == KSERVER_OP_NUM:
        file_id.write('  {{"KSERVER", "GET_VERSION", "GET_CMDS",'
                       + '"GET_STATS", "GET_DEV_STATUS", "GET_RUNNING_SESSIONS"}},\n')
    else:
        file_id.write('  {{"KSERVER", "GET_VERSION", "GET_CMDS",'
                       + '"GET_STATS", "GET_DEV_STATUS", "GET_RUNNING_SESSIONS", ')
        for i in range(KSERVER_OP_NUM, max_op_num-1):
                file_id.write('"", ')
        file_id.write('""}},\n')
    
    for device in devices:
        op_num = len(device.operations)
        file_id.write('  {{"' +  device.name.upper() + '", ')
        for idx, operation in enumerate(device.operations):
            if idx == op_num-1:
                break
        
            file_id.write('"' + operation["name"] + '", ')
        if op_num == max_op_num:
            file_id.write('"' + device.operations[op_num-1]["name"] + '"}},\n')
        else:
            file_id.write('"' + device.operations[op_num-1]["name"] + '", ')
            for i in range(op_num, max_op_num-1):
                file_id.write('"", ')
            file_id.write('""}},\n')
    
    file_id.write('}};\n\n' )
    return

