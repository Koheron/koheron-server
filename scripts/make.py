#! /usr/bin/python

# (c) Koheron

import os
import sys
import yaml

from devgen import generate

def main(argv):
    cmd = argv[0]
    tmp_dir = argv[2]
    if not os.path.exists(tmp_dir):
        os.makedirs(tmp_dir)

    with open(argv[1]) as config_file:
        config = yaml.load(config_file)

    if cmd == '--generate':
        generate(config['drivers'], '.', tmp_dir)

    else:
        raise ValueError('Unknown command')

if __name__ == "__main__":
    main(sys.argv[1:])
