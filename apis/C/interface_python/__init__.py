# Init file for the Koheron API package
#
# (c) Koheron

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
