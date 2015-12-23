>>OPEN
#----------------------------------------
#   Do nothing, DevMem already opened by KServer
#
#----------------------------------------
@{
    return 0;
@}

>>ADD_MEMORY_MAP
#----------------------------------------
#   Add memory map
#
#   - args.device_addr: unsigned int
#   - args.map_size: unsigned int
#----------------------------------------
@{
    Klib::MemMapID map_id = THIS->dev_mem.AddMemoryMap(args.device_addr, 
                                                       args.map_size);
    
    if(static_cast<int>(map_id) < 0) {
        if(SEND_CSTR("ERR\n") < 0) {
            return -1;
        }
        
        kserver->syslog.print(SysLog::DEBUG, "[S] ERR\n");
    } else {
        if(SEND<uint32_t>(static_cast<uint32_t>(map_id)) < 0) {
            return -1;
        }
        
        kserver->syslog.print(SysLog::DEBUG, "[S] %u\n",
                              static_cast<uint32_t>(map_id));
    }
    
    return static_cast<int>(map_id);
@}

>>RM_MEMORY_MAP
#----------------------------------------
#   Remove a memory map
#
#   - args.mmap_idx: unsigned int
#----------------------------------------
@{
    THIS->dev_mem.RmMemoryMap(args.mmap_idx);
    return 0;
@}

>>READ
#----------------------------------------
#   Read a register
#
#   - args.mmap_idx: unsigned int
#   - args.offset: unsigned int
#----------------------------------------
@{
    uint32_t reg_val = Klib::ReadReg32(THIS->dev_mem.GetBaseAddr(args.mmap_idx) 
                                       + args.offset);

    if(SEND<uint32_t>(reg_val) < 0) {
        return -1;
    }
        
    kserver->syslog.print(SysLog::DEBUG, "[S] %u\n", reg_val);

    return 0;
@}

>>WRITE
#----------------------------------------
#   Write a register
#
#   - args.mmap_idx: unsigned int
#   - args.offset: unsigned int
#   - args.reg_val: unsigned int
#----------------------------------------
@{
    Klib::WriteReg32(THIS->dev_mem.GetBaseAddr(args.mmap_idx) + args.offset, 
                     args.reg_val);
    return 0;
@}

>>WRITE_BUFFER
#----------------------------------------
#   Write a buffer of registers
#
#   - args.mmap_idx: unsigned int
#   - args.offset: unsigned int
#   - args.len_data: unsigned int
#----------------------------------------
@{
    const uint32_t* data_ptr = RCV_HANDSHAKE(args.len_data);
    
    if(data_ptr == nullptr) {
        return -1;
    }

    uint32_t base_add = THIS->dev_mem.GetBaseAddr(args.mmap_idx);
    Klib::WriteBuff32(base_add + args.offset, data_ptr, args.len_data);
    
    return 0;
@}

>>READ_BUFFER
#----------------------------------------
#   Read a buffer of registers
#
#   - args.mmap_idx: unsigned int
#   - args.offset: unsigned int
#   - args.buff_size: unsigned int
#   - args.data_type: string
#----------------------------------------
@{
    int n_bytes_send = SEND_ARRAY<uint32_t>(
            reinterpret_cast<uint32_t*>(THIS->dev_mem.GetBaseAddr(args.mmap_idx) 
                                        + args.offset), args.buff_size);
    
    kserver->syslog.print(SysLog::DEBUG, "[S] [%u bytes]\n", n_bytes_send);
    
    return 0;
@}

>>SET_BIT
#----------------------------------------
#   Set the value of a bit
#
#   - args.mmap_idx: unsigned int
#   - args.offset: unsigned int
#   - args.index: unsigned int
#----------------------------------------
@{
    Klib::SetBit(THIS->dev_mem.GetBaseAddr(args.mmap_idx) + args.offset, 
                 args.index);
    return 0;
@}

>>CLEAR_BIT
#----------------------------------------
#   Clear the value of a bit
#
#   - args.mmap_idx: unsigned int
#   - args.offset: unsigned int
#   - args.index: unsigned int
#----------------------------------------
@{
    Klib::ClearBit(THIS->dev_mem.GetBaseAddr(args.mmap_idx) + args.offset, 
                   args.index);
    return 0;
@}

>>TOGGLE_BIT
#----------------------------------------
#   Toggle the value of a bit
#
#   - args.mmap_idx: unsigned int
#   - args.offset: unsigned int
#   - args.index: unsigned int
#----------------------------------------
@{
    Klib::ToggleBit(THIS->dev_mem.GetBaseAddr(args.mmap_idx) + args.offset, 
                    args.index);
    return 0;
@}

>>MASK_AND
#----------------------------------------
#   Apply a mask to a register with an AND
#
#   - args.mmap_idx: unsigned int
#   - args.offset: unsigned int
#   - args.mask: unsigned int
#----------------------------------------
@{
    Klib::MaskAnd(THIS->dev_mem.GetBaseAddr(args.mmap_idx) + args.offset, 
                  args.mask);
    return 0;
@}

>>MASK_OR
#----------------------------------------
#   Apply a mask to a register with an OR
#
#   - args.mmap_idx: unsigned int
#   - args.offset: unsigned int
#   - args.mask: unsigned int
#----------------------------------------
@{
    Klib::MaskOr(THIS->dev_mem.GetBaseAddr(args.mmap_idx) + args.offset, 
                  args.mask);
    return 0;
@}

>> IS_FAILED
#----------------------------------------
# Return a boolean on the device status
# True if the device cannot operate
#----------------------------------------
@{
    return THIS->dev_mem.IsFailed();
@}

