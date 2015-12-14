# __init__.py
#
# Init file for the devgen package
#
# Thomas Vanderbruggen <thomas@koheron.com>
# 17/03/2015
#
# (c) Koheron 2015

"""devgen
Koheron device generator

Compile device descriptions files and manage code
fragments to generate sources files.
"""

__title__ = 'devgen'
__version__ = '0.0.0'
__author__ = "Koheron SAS"
__license__ = "Proprietary"
__copyright__ = 'Copyright 2015 Koheron SAS'

from generator import Generator
from device import Device

__all__ = [ 'Generator',
            'Device']
            
