# device.py
# Base class for devices
# Thomas Vanderbruggen <thomas@koheron.com>
# (c) Koheron 2015


from kclient import KClient, reference_dict

# --------------------------------------------
# Decorators
# --------------------------------------------

def command(func):
    """ Decorate commands
    
    If the name of the command is CMD_NAME, 
    then the name of the function must be cmd_name.
    """
    def decorator(self, *args, **kwargs):
        self.client.send_command(self.ref['id'], self.ref[func.__name__], 
                                 *(args + tuple(kwargs.values())))
        return func(self, *args, **kwargs)
    return decorator
