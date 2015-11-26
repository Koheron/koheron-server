# __init__.py
#
# Init file for the Koheron API package
#
# Thomas Vanderbruggen <thomas@koheron.com>
# 15/03/2015
#
# (c) Koheron 2015

"""koheron
API to communicate, control, prototype, ...
your Zynq from a remote machine.
"""

__title__ = 'koheron'
__version__ = '0.0.0'
__author__ = "Koheron SAS"
__license__ = "Proprietary"
__copyright__ = 'Copyright 2015 Koheron SAS'

from kclient import KClient
from devmem import Devmem



__all__ = [ 'KClient',
            'Devmem' ]
