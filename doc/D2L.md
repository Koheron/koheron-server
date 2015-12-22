# Device description language (D2L)

## Introduction

A device is the central concept around which the Koheron software is organized. It provides the abstract and uniform representation required to handle the software at the various and heterogeneous levels used by KoheronOS:
* HDL code in the FPGA
* C/C++ code in the Middleware
* Python code in the client side 

At first order a device is associated to a given bitstream in the FPGA, and it defines a *set of operations* that can be realized on the bitstream.

However, note that the tools allow you to define a device in a more general way: it can be associated to several bitstreams, or represent a purely middleware code (e.g. KServer is seen as a device).

## Basic features

The description of a device is performed in YAML. A simplified description of a device ```FRIDGE``` could be:

```yaml
# fridge.yaml
# Description of the FRIDGE device

- name: FRIDGE
  description: Store the food

  operations:
    - name: TURN_ON
      description: Turn the fridge on
    - name: TURN_OFF
      description: Turn the fridge off
    - name: ADD_FOOD
      description: Put food into the fridge
      arguments:
        - name: food_name
          description: Name of the food to store
        - name: position
          description: Where it should be stored in the fridge
        - name: limit_date
          description: Must be eaten before this date
    - name: GET_FOOD
      description: Take food from the fridge
      arguments:
        - name: food_name
          description: Name of the food to take
```
This example shows the main features of a device. It is defined by a ```name``` and a set of ```operations```. For each operation, a set of ```arguments``` is given. 

Therefore, controlling a device in KoheronOS requires to send a command that contains the name of the device, the operation we want it to perform, and the related arguments. As the main tool to interact with devices, *KServer* is designed to receive and handle such commands.

The **Kmake** utility integrates the tools to integrate the device descriptions into KServer and the client Python API.

# Device Description Language

The description above gives the main characteristics of a device, however in practice more informations are required to completely describe a device.

Let consider a real example. Here is a fragment of the pool ```Koheron``` and the device ```DEV_MEM```:
```yaml
# @file koheron.yaml
#
# @brief Pool of Koheron devices
#
# (c) Koheron 2014-2015

name: DEV_MEM
description: Map and access physical memory

operations:
  - name: OPEN
    description: Do nothing, DevMem already opened by KServer
    flags:
      - name: AT_INIT

  - name: ADD_MEMORY_MAP
    description: Add memory map
    return: The ID of the memory map
    arguments:
      - name: device_addr
        type: unsigned int
        description: Physical address of the device
                                
      - name: map_size
        type: unsigned int
        description: Mmap size
                  
  - name: WRITE_BUFFER
    description: Write a buffer of registers
    flags:
      - name: SEND_BUFFER 
        buffer_name: 
          - data
                        
            - name: WRITE_BUFFER
              description: Write a buffer of registers
              flags:
                - name: SEND_BUFFER 
                  buffer_name:
                    - data
              
    arguments:  
      - name: mmap_idx
        type: unsigned int
        cast: Klib::MemMapID
        description: Index of Memory Map  
        
      - name: offset
        type: unsigned int
        description: Offset of the register to write

# Middleware description
includes: # Must be in /middleware
  - drivers/core/wr_register.hpp
                       
objects: 
  #- name: dev_mem
  #  type: Klib::DevMem
    
  # Nb: No object here since the DevMem class is 
  # automatically inserted by devgen (for now ...)
```

### Device
A ```name``` in capital letters must be associated to each device, and a ```description``` of the device purpose is strongly recommended. 

The set of operations is a list introduced by the keyword ```operations```.

A device can be linked to C++ code in the middleware.  The objects of the middleware that must be included into KServer are listed under the keyword ```objects```, each object must be given a ```name``` and a ```type```. The keyword ```includes``` is used to specify the file list where the objects definitions are related functions are set. The base repertory for the files is ```/middleware```.

### Operation

Each operation must be given a ```name``` in capital letters, and a ```description``` is strongly recommended. 

Some ```flags``` can be optionally set: 
* ```AT_INIT```: Describe an operation that must be executed when the client initializes the device.

* ```SEND_BUFFER```: Describe an operation where the client send buffers of data to the device. The name of the buffers must be specified as a lsit under the keyword ```buffer_name```. In this case an argument ```len_$buffer_name$``` will be automatically added to the argument list for each buffer, it contains the buffer length. This allows the implementation of the handshaking protocol for buffer of data transfer.

An optional list of ```arguments``` is associated to each operation. An argument is described by a ```name``` and a ```type``` that must be one of the following elementary types:
* ```unsigned int```
* ```unsigned long```
* ```uint32_t```
* ```uint64_t```
* ```float```
* ```bool```

Optionally, a ```cast``` field can be specified to cast the elementary type into another type (for example an user defined type in the middleware code). A default value can be associated with the argument using the ```default``` keyword. Finally, it is strongly recommended to add a ```description``` of each argument.

# Compilation

Tools to compile devices are provided by the **Kmake** utility.

The device description file can be auto-generated from a Python driver with the Middleware generator (experimental).
