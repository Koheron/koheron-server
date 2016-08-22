#include <kclient.h>
#include <stdio.h>

int main(void)
{
    struct kclient *kcl = kclient_connect("127.0.0.1", 36000);
    dev_id_t id = get_device_id(kcl, "HelloWorld");
    op_id_t add_42_ref = get_op_id(kcl, id, "add_42");

    uint32_t res;
    kclient_send_command(kcl, id, add_42_ref, "I", 58);
    kclient_read_u32(kcl, &res);

    printf("%u\n", res);
    return 0;
}