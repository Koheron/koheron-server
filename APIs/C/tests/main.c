#include <stdlib.h>
#include <stdio.h>

# include <kclient.h>

struct tests_device {
    struct kclient *kcl;        // KClient
    dev_id_t id;                // Device ID
    op_id_t send_std_array_ref; // "SEND_STD_ARRAY" reference
    op_id_t set_buffer_ref;     // "SET_BUFFER" reference
    op_id_t read_int_ref;       // "READ_INT" reference
    op_id_t read_uint_ref;      // "READ_UINT" reference
};

void tests_init(struct tests_device *dev, struct kclient *kcl)
{
    dev->kcl = kcl;
    dev->id = get_device_id(kcl, "TESTS");
    dev->send_std_array_ref = get_op_id(kcl, dev->id, "SEND_STD_ARRAY");
    dev->set_buffer_ref = get_op_id(kcl, dev->id, "SET_BUFFER");
    dev->read_int_ref = get_op_id(kcl, dev->id, "READ_INT");
    dev->read_uint_ref = get_op_id(kcl, dev->id, "READ_UINT");
}

int tests_get_std_array(struct tests_device *dev)
{
    int i;
    float *buff;
    struct command cmd;
    init_command(&cmd, dev->id, dev->send_std_array_ref);

    if (kclient_send(dev->kcl, &cmd) < 0)
        return -1;

    if (kclient_rcv_array(dev->kcl, 10, float) < 0) {
        fprintf(stderr, "Cannot read data\n");
        return -1;
    }

    buff = kclient_get_buffer(dev->kcl, float);
    for (i=0; i<kclient_get_len(dev->kcl, float); i++)
        printf("%i => %f\n", i, buff[i]);

    return 0;
}

#define BUFF_LEN 20

int tests_set_buffer(struct tests_device *dev)
{
    int i;
    uint32_t data[BUFF_LEN];

    for (i=0; i<BUFF_LEN; i++)
        data[i] = i*i;

    if (kclient_send_command(dev->kcl, dev->id, dev->set_buffer_ref, "u", BUFF_LEN) < 0)
        return -1;

    if (kclient_send_array(dev->kcl, data, BUFF_LEN) < 0)
        return -1;

    return 0;
}

int tests_read_int(struct tests_device *dev)
{
    int8_t rcv_int;
    struct command cmd;
    init_command(&cmd, dev->id, dev->read_int_ref);

    if (kclient_send(dev->kcl, &cmd) < 0)
        return -1;

    if (kclient_read_int(dev->kcl, &rcv_int))
        return -1;

    printf("Received int %i\n", rcv_int);
    return 0;
}

int tests_read_uint(struct tests_device *dev)
{
    uint32_t rcv_uint;
    struct command cmd;
    init_command(&cmd, dev->id, dev->read_uint_ref);

    if (kclient_send(dev->kcl, &cmd) < 0)
        return -1;

    if (kclient_read_u32(dev->kcl, &rcv_uint))
        return -1;

    printf("Received int %i\n", rcv_uint);
    return 0;
}

int main(void)
{
    struct tests_device dev;

    struct kclient *kcl = kclient_connect("127.0.0.1", 36100);

    if (kcl == NULL) {
        fprintf(stderr, "Can't connect to server\n");
        exit(EXIT_FAILURE);
    }

    tests_init(&dev, kcl);
    tests_get_std_array(&dev);
    tests_set_buffer(&dev);
    tests_read_int(&dev);
    tests_read_uint(&dev);

    kclient_shutdown(kcl);
    return 0;
}