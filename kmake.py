#! /usr/bin/python

# Build utilitaries interface for Koheron server
#
# (c) Koheron

import sys
from devgen import Device, Generator

usage = \
"""
usage: kmake.py <command> [<args>]

The available commands are:
  kserver      Build KServer: sources generation and compilation
  fragments    Code fragments manipulations
  
Use kmake.py <command> --help for the detailed usage of a given command
"""
 
# -------------------------------------
# Fragments
# -------------------------------------

fragments_usage = \
"""
usage: kmake.py fragments [-h|--help] [<options> <YAML filename>]

options:
  -t, --template        Generate a template file to edit the C++
                        code interfacing KServer with the middleware
"""
        
def kmake_fragments(argv):
    """ Code fragments manipulation
    """
    if len(argv) == 0:
        raise ValueError("No arguments for kmake fragments")
    
    if (argv[0] == '-h') or (argv[0] == '--help'):
        print fragments_usage
    elif (argv[0] == '-t') or (argv[0] == '--template'):
        if len(argv) != 2:
            raise ValueError("Template generation requires YAML filename")
            
        device = Device(argv[1])
        device.BuildFragmentTpl()
    else:
        print fragments_usage
        raise ValueError("Invalid option")
        return
        
    return
        
# -------------------------------------
# KServer
# -------------------------------------

kserver_usage = \
"""
usage: kmake.py kserver [-h|--help] [<options> [cfg_file]]

options:
 -g, --generate            Generate the source files
 -c, --generate-compile    Generate the sources and compile KServer
"""
        
def kmake_kserver(argv):
    """ Build KServer
    """
    if len(argv) == 0:
        raise ValueError("kmake kserver requires at least 1 argument")
    
    if (argv[0] == '-g') or (argv[0] == '--generate'):
        # Generate the source files  
        Generator(argv[1], middleware_path=_parse_argv_gen_comp(argv))
        return
    elif (argv[0] == '-c') or (argv[0] == '--generate-compile'):
        # Generate and compile the source files           
        generator = Generator(argv[1], middleware_path=_parse_argv_gen_comp(argv))
        generator.compile()
        return
    elif (argv[0] == '-h') or (argv[0] == '--help'):
        print kserver_usage
    else:
        print kserver_usage
        raise ValueError("Invalid option")
        return
        
def _parse_argv_gen_comp(argv):
    if len(argv) == 1:
        raise ValueError("Configuration file required for source file generation")
            
    if len(argv) == 3:
        midware_path = argv[2]
    else:
        midware_path = 'middleware'
        
    if len(argv) > 3:
        raise ValueError("Too many arguments for source file generation")
        
    return midware_path
    
# -------------------------------------
# Main
# -------------------------------------

def main(argv):
    if len(argv) == 0:
        print "Please select a command"
        print usage
        return
    
    if argv[0] == 'kserver':
        kmake_kserver(argv[1:])
        return
        
    if argv[0] == 'fragments':
        kmake_fragments(argv[1:])
        return

    elif argv[0] == '-h' or argv[0] == '--help':
        print usage
        return
        
    else:
        raise ValueError("Unknown command " + argv[0] + "\n\n" + usage )

if __name__ == "__main__":
        main(sys.argv[1:])
