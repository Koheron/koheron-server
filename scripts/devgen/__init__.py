# Init file for the devgen package
#
# (c) Koheron

"""devgen
Koheron device generator

Compile device descriptions files and manage code
fragments to generate sources files.
"""

__title__ = 'devgen'
__version__ = '1.0.0'
__author__ = "Koheron SAS"
__license__ = "AGPL"
__copyright__ = 'Copyright 2015 Koheron SAS'

from device import Device
from parser_generator import parser_generator

__all__ = [ 'Generator',
            'Device']
            
