# Loads and generates devices descriptions. Generates source code.
#
# (c) Koheron

import yaml
import os
import jinja2

import autogen_implementation
from middleware_handler import MiddlewareHandler

def GetClassName(dev_name):
    """ Build the C++ kdevice class name

    Args:
        - dev_name: Name of the device
    """
    return 'KS_' + dev_name[0].upper() + dev_name[1:].lower()

# Helper function to test fields
def IsFlags(operation):
    """ Test whether flags are defined for a given operation """
    return ("flags" in operation) and (operation["flags"] != None)

def IsArgs(operation):
    """ Test whether arguments are defined for a given operation """
    return ("arguments" in operation) and (operation["arguments"] != None)

def IsDefaultVal(arg):
    """ Test whether a default value is defined for an argument """
    return ("default" in arg) and (arg["default"] != None) and (len(arg["default"]) != 0)

def IsReturn(operation):
    """ Test whether a return description are defined for a given operation """
    return ("return" in operation) and (operation["return"] != None)
    
def IsDesc(operation):
    """ Test whether a description is associated to a given operation """
    return ("description" in operation) and (operation["description"] != None)

class Device:
    """ Device
    Loads and stores the description and fragments of a device.
    Generate the code associated with the device.
    """
    def __init__(self, path, base_dir='.'):
        """ Load a device

        If a C++ header file is provided, the device description data and the
        fragments will be automatically generated from it.

        Args:
            - path: Device filename (either a YAML or a C++ header file)
            - base_dir: XXX Do I really need that ?
        """    
        extension = os.path.basename(path).split('.')[1]
        path = os.path.join(base_dir, path)

        if extension == "hpp" or extension == "h":
            print "Generating device description for " + path + "..."
            mid_handler = MiddlewareHandler(path)
            self._data = mid_handler.get_device_data()
            self.fragments = mid_handler.get_fragments()
            
            self.operations = Operations(self._data["operations"])
            self.name = self._data["name"]
            self.class_name = GetClassName(self.name)
            self.description = self._data["description"]
        else:
            raise ValueError("Invalid device file " + os.path.basename(path))

        self.objects = Objects(self._data["objects"])
        self.includes = Includes(self._data["includes"])

    def Generate(self, directory):   
        """ Generate source code
        Args:
            directory: Directory where the generated sources must be saved
        """
        self._render_ks_device_header(directory)          # Generate KServer header file (hpp)
        autogen_implementation.Generate(self, directory)  # Generate KServer implementation (cpp)

    def _render_ks_device_header(self, directory):
        template_filename = 'devgen/templates/ks_device.hpp'

        header_renderer = jinja2.Environment(
          loader = jinja2.FileSystemLoader(os.path.abspath('.'))
        )

        template = header_renderer.get_template(template_filename)

        header_filename = os.path.join(directory, 
                                       self.class_name.lower() + '.hpp')
        output = file(header_filename, 'w')
        output.write(template.render(device=self))
        output.close()

class Operations:
    def __init__(self, operations):
        self._operations = operations

    def __len__(self):
        return len(self._operations)

    def __getitem__(self, index):
        if index >= len(self):
            raise IndexError
        return self._operations[index]

    def is_valid_op(self, op_name):
        """ Test whether an operation is valid
        Args:
            - op_name: Name of the operation to check
        """
        for operation in self:
            if operation["name"] == op_name:
                return True

        return False

class Objects:
    """ Objects from external API """

    def __init__(self, objects):
        if objects == None:
            self._objects = []
        else:
            self._objects = objects

    def __len__(self):
        return len(self._objects)

    def __getitem__(self, index):
        if index >= len(self):
            raise IndexError
        return self._objects[index]

class Includes:
    """ API files to include """
    def __init__(self, includes):
        if includes == None:
            self._includes = []
        else:
            self._includes = includes

    def __len__(self):
        return len(self._includes)

    def __getitem__(self, index):
        if index >= len(self):
            raise IndexError
        return self._includes[index]