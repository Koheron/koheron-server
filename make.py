#! /usr/bin/python

# (c) Koheron

import os
import sys
import yaml
import jinja2
import time
import subprocess

from devgen import Generator

SRC_DIR = 'tmp'

def render_makefile(obj_files):
    print "Generate Makefile"
    makefile_renderer = jinja2.Environment(
      loader = jinja2.FileSystemLoader(os.path.abspath('.'))
    )

    template = makefile_renderer.get_template('core/Makefile')
    date = time.strftime("%c")
    makefile_filename = os.path.join(SRC_DIR, 'Makefile')

    output = file(makefile_filename, 'w')
    output.write(template.render(date=date, objs_list=obj_files))
    output.close()

def compile(config_filename, middleware_path):
    with open(config_filename) as config_file:    
        config = yaml.load(config_file)

    print "Compiling ..."

    if "host" in config:
        subprocess.check_call("make -C tmp TARGET_HOST=" + config["host"] + " clean all", shell=True)
    elif "cross-compile" in config:
        subprocess.check_call("make -C tmp CROSS_COMPILE=" + config["cross-compile"] + " clean all", shell=True)
    else:
        raise ValueError("Specify a target host or a cross-compilation toolchain")

def main(argv):
    if (argv[0] == '-g') or (argv[0] == '--generate'):
        Generator(argv[1], argv[2], SRC_DIR)
        return

    elif (argv[0] == '-c') or (argv[0] == '--generate-compile'):
        generator = Generator(argv[1], argv[2], SRC_DIR)
        render_makefile(generator.obj_files)
        compile(argv[1], argv[2])
        return

    else:
        raise ValueError("Unknown command " + argv[0] + "\n\n" + usage )

if __name__ == "__main__":
        main(sys.argv[1:])
