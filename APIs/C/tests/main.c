#include <stdlib.h>
#include <stdio.h>

# include <kclient.h>

struct tests_device {
    struct kclient *kcl;  // KClient
    dev_id_t id;          // Device ID
    op_id_t send_std_array_ref; // "GET_CSTR" reference
};

struct tests_device* tests_init(struct kclient * kcl)
{
    struct tests_device *ret_dev = malloc(sizeof(*ret_dev));
    
    if (ret_dev == NULL) {
        fprintf(stderr, "Can't allocate tests_device memory\n");
        return NULL;
    }

    struct tests_device dev = {
        .kcl = kcl,
        .id = get_device_id(kcl, "TESTS"),
        .send_std_array_ref = get_op_id(kcl, dev.id, "SEND_STD_ARRAY")
    };

    memcpy(ret_dev, &dev, sizeof(struct tests_device));
    return ret_dev;
}

int tests_get_std_array(struct tests_device *dev)
{
    int i;
    int bytes_read;
    struct rcv_buff rcv_buffer;
    uint32_t *buff;

    struct command *cmd = init_command(dev->id);
    cmd->op_ref = dev->send_std_array_ref;

    if (kclient_send(dev->kcl, cmd) < 0)
        return -1;

    bytes_read = kclient_rcv_n_bytes(dev->kcl, &rcv_buffer, sizeof(uint32_t) * 10);

    if (bytes_read < 0) {
        fprintf(stderr, "Cannot read data\n");
        return -1;
    }

    buff = (uint32_t *) rcv_buffer.buffer;
    for (i=0; i<rcv_buffer.current_len / sizeof(uint32_t); i++)
        printf("%i => %u\n", i, buff[i]);

    free_command(cmd);
    return 0;
}

int main(void)
{
    struct kclient *kcl = kclient_connect("127.0.0.1", 36100);

    if (kcl == NULL) {
        fprintf(stderr, "Can't connect to server\n");
        exit(EXIT_FAILURE);
    }

    struct tests_device *dev = tests_init(kcl);
    tests_get_std_array(dev);

    kclient_shutdown(kcl);
    return 0;
}