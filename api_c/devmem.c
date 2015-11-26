/** 
 * Implementation of devmem.h
 *
 * (c) Koheron
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "definitions.h"
#include "kclient.h"
 
struct devmem {
    struct kclient * kcl;    // KClient
    
    dev_id_t id;             // Device ID
    
    op_id_t open_ref;        // "OPEN" reference
    op_id_t add_mem_map_ref; // "ADD_MEMORY_MAP" reference
    op_id_t read_ref;        // "READ" reference
    op_id_t write_ref;       // "WRITE" reference
    op_id_t write_buff_ref;  // "WRITE_BUFFER" reference
    op_id_t read_buff_ref;   // "READ_BUFFER" reference  
    
    struct command * cmd;
};

/*
 * Internal functions
 */
 
static int is_devmem_ok(struct devmem * dvm)
{
    if (dvm->id < 0) {
        fprintf(stderr, "Can't find device DEV_MEM\n");
        return 0;
    }
    
    if (dvm->open_ref < 0) {
        fprintf(stderr, "Can't find operation OPEN\n");
        return 0;
    }
    
    if (dvm->add_mem_map_ref < 0) {
        fprintf(stderr, "Can't find operation ADD_MEMORY_MAP\n");
        return 0;
    }
    
    if (dvm->read_ref < 0) {
        fprintf(stderr, "Can't find operation READ\n");
        return 0;
    }
    
    if (dvm->write_ref < 0) {
        fprintf(stderr, "Can't find operation WRITE\n");
        return 0;
    }
    
    if (dvm->write_buff_ref < 0) {
        fprintf(stderr, "Can't find operation WRITE_BUFFER\n");
        return 0;
    }
    
    if (dvm->read_buff_ref < 0) {
        fprintf(stderr, "Can't find operation READ_BUFFER\n");
        return 0;
    }
    
    if (dvm->cmd == NULL) {
        fprintf(stderr, "Can't initialize command\n");
        return 0;
    }
    
    return 1;
}

static int open_devmem(struct devmem * dvm)
{
    reset_command(dvm->cmd, dvm->open_ref);
     
    if (kclient_send(dvm->kcl, dvm->cmd) < 0) {
        return -1;
    }
    
    return 0;
}

/*
 * External functions
 */

KOHERON_LIB_EXPORT
struct devmem * dev_mem_init(struct kclient * kcl)
{   
    struct devmem * ret_dvm = malloc(sizeof(*ret_dvm));
    
    if (ret_dvm == NULL) {
        fprintf(stderr, "Can't allocate Devmem memory\n");
        return NULL;
    }
    
    struct devmem dvm = {
        .kcl = kcl,
        .id = get_device_id(kcl, "DEV_MEM"),
        .open_ref = get_op_id(kcl, dvm.id, "OPEN"),
        .add_mem_map_ref = get_op_id(kcl, dvm.id, "ADD_MEMORY_MAP"),
        .read_ref = get_op_id(kcl, dvm.id, "READ"),
        .write_ref = get_op_id(kcl, dvm.id, "WRITE"),
        .write_buff_ref = get_op_id(kcl, dvm.id, "WRITE_BUFFER"),
        .read_buff_ref = get_op_id(kcl, dvm.id, "READ_BUFFER")
    };
    
    dvm.cmd = init_command(dvm.id);
    
    if (is_devmem_ok(&dvm) == 0) {
        if (dvm.cmd != NULL) {
            free_command(dvm.cmd);
        }
        
        return NULL;
    }
    
    if (open_devmem(&dvm) < 0) {
        free_command(dvm.cmd);
        return NULL;
    }
    
    memcpy(ret_dvm, &dvm, sizeof(struct devmem));
    
    return ret_dvm;
}

KOHERON_LIB_EXPORT
int dev_mem_add_map(struct devmem * dvm, int addr, int size)
{
    reset_command(dvm->cmd, dvm->add_mem_map_ref);
    
    if (add_parameter(dvm->cmd, addr) < 0) {
        return -1;
    }
    
    if (add_parameter(dvm->cmd, size) < 0) {
        return -1;
    }
     
    if (kclient_send(dvm->kcl, dvm->cmd) < 0) {
        return -1;
    }
    
    // TODO Check reception
    
    return 0;
}

KOHERON_LIB_EXPORT
void dev_mem_exit(struct devmem * dvm)
{
    if (dvm != NULL) {
        free_command(dvm->cmd);
        free(dvm);
        dvm = NULL;
    }
}



