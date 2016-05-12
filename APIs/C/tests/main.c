#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

# include <kclient.h>

struct tests_device {
    struct kclient *kcl;         // KClient
    dev_id_t id;                 // Device ID
    op_id_t rcv_many_params_ref; // "RCV_MANY_PARAMS" reference
    op_id_t read_uint_ref;       // "READ_UINT" reference
    op_id_t read_int_ref;        // "READ_INT" reference
    op_id_t set_float_ref;       // "SET_FLOAT" reference
    op_id_t send_std_array_ref;  // "SEND_STD_ARRAY" reference
    op_id_t set_buffer_ref;      // "SET_BUFFER" reference
};

void tests_init(struct tests_device *dev, struct kclient *kcl)
{
    dev->kcl = kcl;
    dev->id = get_device_id(kcl, "TESTS");
    dev->rcv_many_params_ref = get_op_id(kcl, dev->id, "RCV_MANY_PARAMS");
    dev->read_uint_ref = get_op_id(kcl, dev->id, "READ_UINT");
    dev->read_int_ref = get_op_id(kcl, dev->id, "READ_INT");
    dev->set_float_ref = get_op_id(kcl, dev->id, "SET_FLOAT");
    dev->send_std_array_ref = get_op_id(kcl, dev->id, "SEND_STD_ARRAY");
    dev->set_buffer_ref = get_op_id(kcl, dev->id, "SET_BUFFER");

    assert(dev->send_std_array_ref    >= 0 
           && dev->set_buffer_ref     >= 0
           && dev->read_int_ref       >= 0
           && dev->read_uint_ref      >= 0);
}

bool test_send_many_params(struct tests_device *dev)
{
    bool is_ok;

    if (kclient_send_command(dev->kcl, dev->id, dev->rcv_many_params_ref, "uufb", 42, 2048, 3.14, true) < 0
        || kclient_read_bool(dev->kcl, &is_ok))
        return false;

    return is_ok;
}

bool test_read_uint(struct tests_device *dev)
{
    uint32_t rcv_uint;

    if (kclient_send_command(dev->kcl, dev->id, dev->read_uint_ref, "") < 0
        || kclient_read_u32(dev->kcl, &rcv_uint))
        return false;

    return rcv_uint == 42;
}

bool test_read_int(struct tests_device *dev)
{
    int rcv_int;

    if (kclient_send_command(dev->kcl, dev->id, dev->read_int_ref, "") < 0
        || kclient_read_int(dev->kcl, &rcv_int))
        return false;

    return rcv_int == -42;
}

bool test_set_float(struct tests_device *dev)
{
    bool is_ok;

    if (kclient_send_command(dev->kcl, dev->id, dev->set_float_ref, "f", 12.5) < 0
        || kclient_read_bool(dev->kcl, &is_ok))
        return false;

    return is_ok;
}

bool test_rcv_std_array(struct tests_device *dev)
{
    int i;
    float *buff;

    if (kclient_send_command(dev->kcl, dev->id, dev->send_std_array_ref, "") < 0
        || kclient_rcv_array(dev->kcl, 10, float) < 0)
        return false;

    buff = kclient_get_buffer(dev->kcl, float);
    for (i=0; i<kclient_get_len(dev->kcl, float); i++)
        if (buff[i] != i*i)
            return false;

    return true;
}

#define BUFF_LEN 10

bool test_send_buffer(struct tests_device *dev)
{
    int i;
    uint32_t data[BUFF_LEN];
    bool is_ok;

    for (i=0; i<BUFF_LEN; i++)
        data[i] = i*i;

    if (kclient_send_command(dev->kcl, dev->id, dev->set_buffer_ref, "u", BUFF_LEN) < 0
        || kclient_send_array(dev->kcl, data, BUFF_LEN) < 0
        || kclient_read_bool(dev->kcl, &is_ok))
        return false;

    return is_ok;
}

// http://stackoverflow.com/questions/1961209/making-some-text-in-printf-appear-in-green-and-red
#define RESET       "\x1B[0m"
#define BOLDRED     "\033[1m\033[31m"
#define BOLDGRN     "\033[1m\033[32m"
#define BOLDWHITE   "\033[1m\033[37m"

#define SETUP                                                       \
    struct tests_device dev;                                        \
    struct kclient *kcl = kclient_connect("127.0.0.1", 36000);      \
                                                                    \
    if (kcl == NULL) {                                              \
        fprintf(stderr, "Can't connect to server\n");               \
        exit(EXIT_FAILURE);                                         \
    }                                                               \
                                                                    \
    tests_init(&dev, kcl);                                          \

#define TEARDOWN                                                    \
    kclient_shutdown(kcl);

#define TEST(NAME)                                                  \
do {                                                                \
    SETUP                                                           \
    if (test_##NAME(&dev)) {                                        \
        printf(BOLDGRN "\xE2\x9C\x93" RESET " %s\n", #NAME);        \
    } else {                                                        \
        printf(BOLDRED "\xE2\x98\x93" RESET " %s\n", #NAME);        \
        tests_fail++;                                               \
    }                                                               \
    tests_num++;                                                    \
    TEARDOWN                                                        \
} while(0);

int main(void)
{
    int tests_num  = 0;
    int tests_fail = 0;

    printf(BOLDWHITE "\nStart tests\n\n" RESET);

    TEST(send_many_params)
    TEST(read_uint)
    TEST(read_int)
    TEST(set_float)
    TEST(rcv_std_array)
    TEST(send_buffer)
    
    if (tests_fail == 0) {
        printf("\n" BOLDGRN "OK" RESET " -- %u tests passed\n", tests_num);
    } else {
        printf("\n" BOLDRED "FAIL" RESET " -- %u tests passed - %u failed\n",
               tests_num - tests_fail, tests_fail);
        exit(EXIT_FAILURE);
    }

    return 0;
}