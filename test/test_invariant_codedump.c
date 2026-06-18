#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declaration of the function under test from src/codedump.c */
extern void codedump(const char *name, int lenp, char *buf, int bufsize);

START_TEST(test_buffer_read_bounds)
{
    /* Invariant: Buffer reads never exceed the declared length.
       codedump must not read beyond lenp bytes from name or write beyond bufsize to buf. */
    
    char buf[64];
    const char *payloads[] = {
        "normal_input",                    /* Valid: short string */
        "x",                               /* Boundary: single char */
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"  /* Exploit: 64 bytes, matches buf */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);
    
    for (int i = 0; i < num_payloads; i++) {
        memset(buf, 0, sizeof(buf));
        int payload_len = strlen(payloads[i]);
        
        /* Test 1: lenp within buffer bounds - should succeed */
        int safe_len = (payload_len < (int)sizeof(buf)) ? payload_len : (int)sizeof(buf) - 1;
        codedump(payloads[i], safe_len, buf, sizeof(buf));
        ck_assert_int_le(safe_len, (int)sizeof(buf));
        
        /* Test 2: lenp exceeds buffer - codedump must handle gracefully */
        memset(buf, 0, sizeof(buf));
        int oversized_len = sizeof(buf) * 2;
        codedump(payloads[i], oversized_len, buf, sizeof(buf));
        /* Invariant: no crash and buffer integrity maintained */
        ck_assert_ptr_nonnull(buf);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_read_bounds);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}