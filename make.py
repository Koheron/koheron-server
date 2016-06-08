#! /usr/bin/python

# (c) Koheron

import os
import sys
import yaml
import subprocess

from devgen import Generator


def compile(config_filename, middleware_path):
    with open(config_filename) as config_file:    
        config = yaml.load(config_file)

    print "Compiling ..."

    if "host" in config:
        subprocess.check_call("make -C tmp/server TARGET_HOST=" + config["host"] + " clean all", shell=True)
    elif "cross-compile" in config:
        subprocess.check_call("make -C tmp/server CROSS_COMPILE=" + config["cross-compile"] + " clean all", shell=True)
    else:
        raise ValueError("Specify a target host or a cross-compilation toolchain")

def main(argv):
    if (argv[0] == '-g') or (argv[0] == '--generate'):
        Generator(argv[1], argv[2])
        return

    elif (argv[0] == '-c') or (argv[0] == '--generate-compile'):
        generator = Generator(argv[1], argv[2])
        compile(argv[1], argv[2])
        return

    else:
        raise ValueError("Unknown command " + argv[0] + "\n\n" + usage )

if __name__ == "__main__":
        main(sys.argv[1:])
