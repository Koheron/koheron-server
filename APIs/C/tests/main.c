#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#if defined (__linux__)
  #include <time.h>
#endif

#include <kclient.h>

struct tests_device {
    struct kclient *kcl;         // KClient
    dev_id_t id;                 // Device ID
    op_id_t rcv_many_params_ref; // "RCV_MANY_PARAMS" reference
    op_id_t read_uint_ref;       // "READ_UINT" reference
    op_id_t read_int_ref;        // "READ_INT" reference
    op_id_t set_float_ref;       // "SET_FLOAT" reference
    op_id_t send_std_vector_ref; // "SEND_STD_VECTOR" reference
    op_id_t send_std_array_ref;  // "SEND_STD_ARRAY" reference
    op_id_t send_c_array1_ref;   // "SEND_C_ARRAY1" reference
    op_id_t send_c_array2_ref;   // "SEND_C_ARRAY2" reference
    op_id_t set_buffer_ref;      // "SET_BUFFER" reference
    op_id_t get_cstr_ref;        // "GET_CSTR" reference
};

void tests_init(struct tests_device *dev, struct kclient *kcl)
{
    dev->kcl = kcl;
    dev->id = get_device_id(kcl, "TESTS");
    dev->rcv_many_params_ref = get_op_id(kcl, dev->id, "RCV_MANY_PARAMS");
    dev->read_uint_ref = get_op_id(kcl, dev->id, "READ_UINT");
    dev->read_int_ref = get_op_id(kcl, dev->id, "READ_INT");
    dev->set_float_ref = get_op_id(kcl, dev->id, "SET_FLOAT");
    dev->send_std_vector_ref = get_op_id(kcl, dev->id, "SEND_STD_VECTOR");
    dev->send_std_array_ref = get_op_id(kcl, dev->id, "SEND_STD_ARRAY");
    dev->send_c_array1_ref = get_op_id(kcl, dev->id, "SEND_C_ARRAY1");
    dev->send_c_array2_ref = get_op_id(kcl, dev->id, "SEND_C_ARRAY2");
    dev->set_buffer_ref = get_op_id(kcl, dev->id, "SET_BUFFER");
    dev->get_cstr_ref = get_op_id(kcl, dev->id, "GET_CSTR");

    assert(dev->send_std_array_ref    >= 0 
           && dev->set_buffer_ref     >= 0
           && dev->read_int_ref       >= 0
           && dev->read_uint_ref      >= 0);
}

bool test_send_many_params(struct tests_device *dev)
{
    bool is_ok;

    if (kclient_send_command(dev->kcl, dev->id, dev->rcv_many_params_ref, 
                             "IIf?", 429496729, 2048, 3.14, true) < 0
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

    return rcv_uint == 301062138;
}

bool test_read_int(struct tests_device *dev)
{
    int rcv_int;

    if (kclient_send_command(dev->kcl, dev->id, dev->read_int_ref, "") < 0
        || kclient_read_int(dev->kcl, &rcv_int))
        return false;

    return rcv_int == -214748364;
}

bool test_set_float(struct tests_device *dev)
{
    bool is_ok;

    if (kclient_send_command(dev->kcl, dev->id, dev->set_float_ref, "f", 12.5) < 0
        || kclient_read_bool(dev->kcl, &is_ok))
        return false;

    return is_ok;
}

bool test_rcv_std_vector(struct tests_device *dev)
{
    int i;
    float *buff;
    int len_exp = 10;
    int len;

    if (kclient_send_command(dev->kcl, dev->id, dev->send_std_vector_ref, "") < 0
        || kclient_rcv_array(dev->kcl, len_exp, float) < 0)
        return false;

    buff = kclient_get_buffer(dev->kcl, float);
    len = kclient_get_len(dev->kcl, float);

    if (len != len_exp)
        return false;

    for (i=0; i<len; i++)
        if (buff[i] != i*i*i)
            return false;

    return true;
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

bool test_rcv_c_array1(struct tests_device *dev)
{
    int i;
    float *buff;
    int len_exp = 10;
    int len;

    if (kclient_send_command(dev->kcl, dev->id, dev->send_c_array1_ref, "I", len_exp) < 0
        || kclient_rcv_array(dev->kcl, 2*len_exp, float) < 0)
        return false;

    buff = kclient_get_buffer(dev->kcl, float);
    len = kclient_get_len(dev->kcl, float);

    if (len != 2*len_exp)
        return false;

    for (i=0; i<len; i++)
        if (buff[i] != ((float)i)/2)
            return false;

    return true;
}

bool test_rcv_c_array2(struct tests_device *dev)
{
    int i;
    float *buff;
    int len_exp = 10;
    int len;

    if (kclient_send_command(dev->kcl, dev->id, dev->send_c_array2_ref, "") < 0
        || kclient_rcv_array(dev->kcl, len_exp, float) < 0)
        return false;

    buff = kclient_get_buffer(dev->kcl, float);
    len = kclient_get_len(dev->kcl, float);

    if (len != len_exp)
        return false;

    for (i=0; i<len; i++)
        if (buff[i] != ((float)i)/4)
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

    if (kclient_send_command(dev->kcl, dev->id, dev->set_buffer_ref, "I", BUFF_LEN) < 0
        || kclient_send_array(dev->kcl, data, BUFF_LEN) < 0
        || kclient_read_bool(dev->kcl, &is_ok))
        return false;

    return is_ok;
}

bool test_read_string(struct tests_device *dev)
{
    if (kclient_send_command(dev->kcl, dev->id, dev->get_cstr_ref, "") < 0
        || kclient_read_string(dev->kcl) < 0)
        return false;

    return strcmp(dev->kcl->buffer, "Hello !") == 0;
}

// http://stackoverflow.com/questions/1961209/making-some-text-in-printf-appear-in-green-and-red
#define RESET       "\x1B[0m"
#define BOLDRED     "\033[1m\033[31m"
#define BOLDGRN     "\033[1m\033[32m"
#define BOLDWHITE   "\033[1m\033[37m"

#define SETUP_TCP                                                   \
    struct tests_device dev;                                        \
    struct kclient *kcl = kclient_connect(IP, port);                \
                                                                    \
    if (kcl == NULL) {                                              \
        fprintf(stderr, "Can't connect to server\n");               \
        exit(EXIT_FAILURE);                                         \
    }                                                               \
                                                                    \
    tests_init(&dev, kcl);

#define SETUP_UNIX                                                  \
    struct tests_device dev;                                        \
    struct kclient *kcl = kclient_unix_connect(unix_sock_path); \
                                                                    \
    if (kcl == NULL) {                                              \
        fprintf(stderr, "Can't connect to server\n");               \
        exit(EXIT_FAILURE);                                         \
    }                                                               \
                                                                    \
    tests_init(&dev, kcl);

#define TEARDOWN                                                    \
    kclient_shutdown(kcl);

#define UNIT_TEST(NAME)                                             \
do {                                                                \
    if (test_##NAME(&dev)) {                                        \
        printf(BOLDGRN "\xE2\x9C\x93" RESET " %s\n", #NAME);        \
    } else {                                                        \
        printf(BOLDRED "\xE2\x98\x93" RESET " %s\n", #NAME);        \
        tests_fail++;                                               \
    }                                                               \
    tests_num++;                                                    \
} while(0);

void unit_tests_tcp(char *IP, unsigned int port)
{
    int tests_num  = 0;
    int tests_fail = 0;

    printf(BOLDWHITE "\nStart TCP socket tests\n\n" RESET);
    SETUP_TCP
    UNIT_TEST(send_many_params)
    UNIT_TEST(read_uint)
    UNIT_TEST(read_int)
    UNIT_TEST(set_float)
    UNIT_TEST(rcv_std_vector)
    UNIT_TEST(rcv_std_array)
    UNIT_TEST(rcv_c_array1)
    UNIT_TEST(rcv_c_array2)
    UNIT_TEST(send_buffer)
    UNIT_TEST(read_string)
    TEARDOWN
    
    if (tests_fail == 0) {
        printf("\n" BOLDGRN "OK" RESET " -- %u tests passed\n", tests_num);
    } else {
        printf("\n" BOLDRED "FAIL" RESET " -- %u tests passed - %u failed\n",
               tests_num - tests_fail, tests_fail);
        exit(EXIT_FAILURE);
    }
}

#if defined (__linux__)
void unit_tests_unix(char *unix_sock_path)
{
    int tests_num  = 0;
    int tests_fail = 0;

    printf(BOLDWHITE "\nStart Unix socket tests\n\n" RESET);
    SETUP_UNIX
    UNIT_TEST(send_many_params)
    UNIT_TEST(read_uint)
    UNIT_TEST(read_int)
    UNIT_TEST(set_float)
    UNIT_TEST(rcv_std_vector)
    UNIT_TEST(rcv_std_array)
    UNIT_TEST(rcv_c_array1)
    UNIT_TEST(rcv_c_array2)
    UNIT_TEST(send_buffer)
    UNIT_TEST(read_string)
    TEARDOWN
    
    if (tests_fail == 0) {
        printf("\n" BOLDGRN "OK" RESET " -- %u tests passed\n", tests_num);
    } else {
        printf("\n" BOLDRED "FAIL" RESET " -- %u tests passed - %u failed\n",
               tests_num - tests_fail, tests_fail);
        exit(EXIT_FAILURE);
    }
}
#endif

#define SPEED_TEST(NAME)                                            \
do {                                                                \
    tic = clock();                                                  \
    for (i=0; i<N; i++)                                             \
        test_##NAME(&dev);                                          \
    toc = clock();                                                  \
                                                                    \
    printf("%s: %f us\n", #NAME,                                    \
           ((double) (toc - tic)) * 1E6 / (N * CLOCKS_PER_SEC));    \
} while(0);

#if defined (__linux__)
void speed_tests_tcp(char *IP, unsigned int port)
{
    unsigned int i;
    unsigned int N = 30000;
    clock_t tic, toc;

    printf(BOLDWHITE "\nTCP socket speed\n" RESET);
    SETUP_TCP
    SPEED_TEST(send_many_params)
    SPEED_TEST(read_uint)
    SPEED_TEST(read_int)
    SPEED_TEST(set_float)
    SPEED_TEST(rcv_std_vector)
    SPEED_TEST(rcv_std_array)
    SPEED_TEST(rcv_c_array1)
    SPEED_TEST(rcv_c_array2)
    SPEED_TEST(send_buffer)
    SPEED_TEST(read_string)
    TEARDOWN
}

void speed_tests_unix(char *unix_sock_path)
{
    unsigned int i;
    unsigned int N = 30000;
    clock_t tic, toc;

    printf(BOLDWHITE "\nUnix socket speed\n" RESET);
    SETUP_UNIX
    SPEED_TEST(send_many_params)
    SPEED_TEST(read_uint)
    SPEED_TEST(read_int)
    SPEED_TEST(set_float)
    SPEED_TEST(rcv_std_vector)
    SPEED_TEST(rcv_std_array)
    SPEED_TEST(rcv_c_array1)
    SPEED_TEST(rcv_c_array2)
    SPEED_TEST(send_buffer)
    SPEED_TEST(read_string)
    TEARDOWN
}
#endif

int main(int argc, char **argv)
{
    char IP[256];
    unsigned int port;

    char *token = strtok(argv[2], ":");
    strcpy(IP, token);
    port = (unsigned int)strtoul(strtok(NULL, ":"), NULL, 10);

    if (strcmp(argv[1], "--unit") == 0) {
        unit_tests_tcp(IP, port);
#if defined (__linux__)
        unit_tests_unix(argv[3]);
#endif
    }
#if defined (__linux__)
    else if (strcmp(argv[1], "--speed") == 0) {
        speed_tests_tcp(IP, port);
        speed_tests_unix(argv[3]);
    }
#endif

    return 0;
}
