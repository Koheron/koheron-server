# Handle code fragments
#
# (c) Koheron

import device as dev_utils

import json
import os.path
               
class FragmentsHandler:
    def __init__(self, device, directory='.', fragments=None):
        self.device = device
        
        if fragments == None: # Load from a frag file
            self.fragments = self._ParseFragments(device, directory)
        else:
            self.fragments = fragments

    # XXX This could be Jinja templatized
    def GenFragTpl(self, directory='.'):
        """ Generate a fragment template file
        """
        filename = os.path.join(directory, self.device.class_name.lower() + '.frag')
        f = open(filename, 'w')

        if len(self.fragments) == 0:
            self._GenFragEmptyTpl(f)
            return
        
        # Print operations
        for operation in self.device.operations:
            # True for a known operation
            # Operation already described in fragments   
            known_op = 0
                
            for frag in self.fragments:
                if operation["name"] == frag['name']:
                    known_op = 1
                    self._PrintOpFragment(f, operation, frag['fragment'])
                    break                
            
            # If unknown operation build empty template
            if known_op == 0:   
                    self._PrintOpFragment(f, operation, \
                        ['    // WRITE YOUR CODE HERE\n', '    return 0;\n'])

        # Print IS FAILED
        known_is_failed = 0

        for frag in self.fragments:
            if frag['name'] == "IS_FAILED":
                known_is_failed = 1
                self._PrintIsFailed(f, frag['fragment'])
                break
                
        if known_is_failed == 0:
            self._PrintIsFailed(f, ['    return false;\n'])

        f.close()

    def _GenFragEmptyTpl(self, file_id):   
        for operation in self.device.operations:
            self._PrintOpFragment(file_id, operation,
                       ['    // WRITE YOUR CODE HERE\n', '    return 0;\n'])
                            
        self._PrintIsFailed(f, 
            ['    // WRITE YOUR CODE HERE\n', '    return false;\n'])
            
    def _PrintOpFragment(self, file_id, operation, frag_lines):
        file_id.write('>>' + operation["name"] + '\n')
           
        if dev_utils.IsArgs(operation) or dev_utils.IsDesc(operation):
            file_id.write('#----------------------------------------\n')
            
            if dev_utils.IsDesc(operation):
                file_id.write('#   ' + operation["description"] + '\n')

            if dev_utils.IsArgs(operation):
                file_id.write('#\n')
                for arg in operation["arguments"]:
                    file_id.write('#   - args.' + arg["name"] + ': '\
                                       + arg["type"] + '\n')
                                            
            file_id.write('#----------------------------------------\n')   
                                     
        file_id.write('@{\n')
        
        for line in frag_lines:
            file_id.write(line)
            
        file_id.write('@}\n\n')
            
    def _PrintIsFailed(self, file_id, frag_lines):
        file_id.write('>> IS_FAILED\n')
        file_id.write('#----------------------------------------\n')
        file_id.write('# Return a boolean on the device status\n') 
        file_id.write('# True if the device cannot operate\n')                                    
        file_id.write('#----------------------------------------\n')                            
        file_id.write('@{\n')
        
        if len(frag_lines) == 0:
            file_id.write('    return false;\n')
        else:
            for line in frag_lines:
                file_id.write(line)
            
        file_id.write('@}\n\n')
            
    def _ParseFragments(self, device, directory='.'):        
        filename = os.path.join(directory, device.class_name.lower() + '.frag')
        
        if not os.path.isfile(filename):
            return []
        
        f = open(filename, "r")

        # Contains the parsing result
        fragments = []

        # Count operators
        cnt_op = 0
        start_fragment = 0
        idx = 0

        for line in f.readlines():
            if line[0] == '#':
                continue 
            elif line[0:2] == '@{':
                start_fragment = 1
                idx = 0
            elif line[0:2] == '@}':
                start_fragment = 0
                idx = 0
            elif start_fragment:
                fragments[cnt_op-1]["fragment"].insert(idx, line)
                idx = idx+1
            elif line[0:2] == '>>':
                op_name = line[2:].split( )[0]
                
                if (op_name != "IS_FAILED"):
                    fragments.insert(cnt_op, {"name":op_name, "fragment":[]})
                    cnt_op = cnt_op + 1
                    
                    if not device.operations.is_valid_op(op_name):
                        raise ValueError('Invalid operation ' + op_name)
                else:
                    fragments.insert(cnt_op, {"name":"IS_FAILED", "fragment":[]})
                    cnt_op = cnt_op + 1

        f.close()
        return fragments

