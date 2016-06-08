#! /usr/bin/python

# (c) Koheron

import sys
from devgen import Device, Generator

usage = \
"""
usage: kmake.py [-h|--help] [<options> [cfg_file]]

options:
 -g, --generate            Generate the source files
 -c, --generate-compile    Generate the sources and compile KServer
"""

def main(argv):
    if (argv[0] == '-g') or (argv[0] == '--generate'):
        Generator(argv[1], argv[2])
        return

    elif (argv[0] == '-c') or (argv[0] == '--generate-compile'):
        generator = Generator(argv[1], argv[2])
        generator.compile()
        return

    elif argv[0] == '-h' or argv[0] == '--help':
        print usage
        return

    else:
        raise ValueError("Unknown command " + argv[0] + "\n\n" + usage )

if __name__ == "__main__":
        main(sys.argv[1:])
