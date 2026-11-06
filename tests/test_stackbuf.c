/* Quoted path so a syntax check without -I still finds the public header. */
#include "../include/stackbuf.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;
static int tests_run = 0;
static int tests_failed = 0;
static const char *current_test = NULL;

static void expect(int condition, const char *file, int line, const char *expr)
{
    if (!condition) {
        printf("FAIL %s %s:%d: %s\n",
               current_test != NULL ? current_test : "?",
               file, line, expr);
        fflush(stdout);
        failures++;
    }
}

#define EXPECT(cond) expect((cond), __FILE__, __LINE__, #cond)

static int semver_major_minor(const char *version)
{
    int parts = 0;

    if (version == NULL) {
        return 0;
    }

    while (parts < 2) {
        if (*version < '0' || *version > '9') {
            return 0;
        }
        while (*version >= '0' && *version <= '9') {
            version++;
        }
        parts++;
        if (parts == 2) {
            return 1;
        }
        if (*version != '.') {
            return 0;
        }
        version++;
    }
    return 0;
}

static void test_version_string_is_semver(void)
{
    EXPECT(semver_major_minor(STACKBUF_VERSION_STRING));
}

static void test_empty_buffer(void)
{
    stackbuf *buf = NULL;
    char *marker = NULL;

    EXPECT(stackbuf_create(NULL, 16) == STACKBUF_INVAL);
    EXPECT(stackbuf_create(&buf, 0) == STACKBUF_INVAL);
    EXPECT(buf == NULL);
    EXPECT(stackbuf_create(&buf, 16) == STACKBUF_OK);
    EXPECT(buf != NULL);
    EXPECT(stackbuf_capacity(buf) == 16);
    EXPECT(stackbuf_used(buf) == 0);
    EXPECT(stackbuf_capacity(NULL) == 0);
    EXPECT(stackbuf_used(NULL) == 0);
    EXPECT(stackbuf_verify(buf) == STACKBUF_OK);
    EXPECT(stackbuf_verify(NULL) == STACKBUF_INVAL);
    EXPECT(stackbuf_mark(buf, &marker) == STACKBUF_OK);
    EXPECT(marker != NULL);
    EXPECT(marker[0] == '\0');
    EXPECT(stackbuf_unmark(buf, &marker) == STACKBUF_OK);
    EXPECT(marker == NULL);
    EXPECT(stackbuf_verify(buf) == STACKBUF_OK);
    EXPECT(stackbuf_destroy(&buf) == STACKBUF_OK);
    EXPECT(buf == NULL);
    EXPECT(stackbuf_destroy(&buf) == STACKBUF_OK);
    EXPECT(stackbuf_destroy(NULL) == STACKBUF_INVAL);
}

static void test_append_concatenates(void)
{
    stackbuf *buf = NULL;
    char *marker = NULL;

    EXPECT(stackbuf_create(&buf, 32) == STACKBUF_OK);
    EXPECT(stackbuf_mark(buf, &marker) == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, "%s", "This") == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, " %s %d", "is", 1) == STACKBUF_OK);
    EXPECT(strcmp(marker, "This is 1") == 0);
    EXPECT(stackbuf_used(buf) == strlen("This is 1"));
    EXPECT(stackbuf_unmark(buf, &marker) == STACKBUF_OK);
    EXPECT(stackbuf_verify(buf) == STACKBUF_OK);
    EXPECT(stackbuf_destroy(&buf) == STACKBUF_OK);
}

static void test_finalise_starts_a_new_string(void)
{
    stackbuf *buf = NULL;
    char *first = NULL;
    char *second = NULL;

    EXPECT(stackbuf_create(&buf, 32) == STACKBUF_OK);
    EXPECT(stackbuf_mark(buf, &first) == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, "ab") == STACKBUF_OK);
    EXPECT(stackbuf_finalise(buf) == STACKBUF_OK);
    EXPECT(strcmp(first, "ab") == 0);
    EXPECT(stackbuf_mark(buf, &second) == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, "cd") == STACKBUF_OK);
    EXPECT(strcmp(first, "ab") == 0);
    EXPECT(strcmp(second, "cd") == 0);
    EXPECT(stackbuf_verify(buf) == STACKBUF_INVAL);
    EXPECT(stackbuf_reset(buf) == STACKBUF_OK);
    EXPECT(stackbuf_used(buf) == 0);
    EXPECT(stackbuf_verify(buf) == STACKBUF_OK);
    EXPECT(stackbuf_destroy(&buf) == STACKBUF_OK);
}

static void test_nested_mark_rewinds_inner_text(void)
{
    stackbuf *buf = NULL;
    char *outer = NULL;
    char *inner = NULL;

    EXPECT(stackbuf_create(&buf, 64) == STACKBUF_OK);
    EXPECT(stackbuf_mark(buf, &outer) == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, "This") == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, " is a") == STACKBUF_OK);

    EXPECT(stackbuf_mark(buf, &inner) == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, "This") == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, " is a") == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, " test") == STACKBUF_OK);
    EXPECT(stackbuf_finalise(buf) == STACKBUF_OK);
    EXPECT(strcmp(inner, "This is a test") == 0);
    EXPECT(stackbuf_unmark(buf, &inner) == STACKBUF_OK);
    EXPECT(inner == NULL);

    EXPECT(stackbuf_append(buf, " test also") == STACKBUF_OK);
    EXPECT(stackbuf_finalise(buf) == STACKBUF_OK);
    EXPECT(strcmp(outer, "This is a test also") == 0);
    EXPECT(stackbuf_unmark(buf, &outer) == STACKBUF_OK);
    EXPECT(stackbuf_verify(buf) == STACKBUF_OK);
    EXPECT(stackbuf_destroy(&buf) == STACKBUF_OK);
}

static void test_overflow_leaves_the_string_unchanged(void)
{
    stackbuf *buf = NULL;
    char *marker = NULL;

    EXPECT(stackbuf_create(&buf, 8) == STACKBUF_OK);
    EXPECT(stackbuf_mark(buf, &marker) == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, "hello") == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, "world") == STACKBUF_NOSPACE);
    EXPECT(strcmp(marker, "hello") == 0);
    EXPECT(stackbuf_used(buf) == 5);
    EXPECT(stackbuf_append(buf, "!") == STACKBUF_OK);
    EXPECT(strcmp(marker, "hello!") == 0);
    EXPECT(stackbuf_destroy(&buf) == STACKBUF_OK);
}

static void test_capacity_one_holds_only_an_empty_string(void)
{
    stackbuf *buf = NULL;
    char *marker = NULL;

    EXPECT(stackbuf_create(&buf, 1) == STACKBUF_OK);
    EXPECT(stackbuf_mark(buf, &marker) == STACKBUF_OK);
    EXPECT(stackbuf_append(buf, "x") == STACKBUF_NOSPACE);
    EXPECT(marker[0] == '\0');
    EXPECT(stackbuf_append(buf, "%s", "") == STACKBUF_OK);
    EXPECT(stackbuf_finalise(buf) == STACKBUF_OK);
    EXPECT(stackbuf_used(buf) == 1);
    EXPECT(stackbuf_append(buf, "x") == STACKBUF_NOSPACE);
    EXPECT(stackbuf_mark(buf, &marker) == STACKBUF_NOSPACE);
    EXPECT(stackbuf_destroy(&buf) == STACKBUF_OK);
}

static void test_buffers_are_independent(void)
{
    stackbuf *left = NULL;
    stackbuf *right = NULL;
    char *left_mark = NULL;
    char *right_mark = NULL;
    stackbuf *kept;

    EXPECT(stackbuf_create(&left, 16) == STACKBUF_OK);
    EXPECT(stackbuf_create(&right, 16) == STACKBUF_OK);
    kept = left;
    EXPECT(stackbuf_create(&left, 0) == STACKBUF_INVAL);
    EXPECT(left == kept);
    EXPECT(stackbuf_append(left, "one") == STACKBUF_OK);
    EXPECT(stackbuf_append(right, "two") == STACKBUF_OK);
    EXPECT(stackbuf_mark(left, &left_mark) == STACKBUF_OK);
    EXPECT(strcmp(left_mark, "") == 0);
    EXPECT(stackbuf_reset(left) == STACKBUF_OK);
    EXPECT(stackbuf_mark(left, &left_mark) == STACKBUF_OK);
    EXPECT(stackbuf_append(left, "one") == STACKBUF_OK);
    EXPECT(stackbuf_mark(right, &right_mark) == STACKBUF_OK);
    EXPECT(strcmp(right_mark, "") == 0);
    EXPECT(stackbuf_unmark(right, &left_mark) == STACKBUF_INVAL);
    EXPECT(left_mark != NULL);
    EXPECT(strcmp(left_mark, "one") == 0);
    EXPECT(stackbuf_used(right) == 3);
    EXPECT(stackbuf_destroy(&left) == STACKBUF_OK);
    EXPECT(stackbuf_destroy(&right) == STACKBUF_OK);
}

static void test_invalid_arguments(void)
{
    stackbuf *buf = NULL;
    char *marker = NULL;

    EXPECT(stackbuf_create(&buf, 8) == STACKBUF_OK);
    EXPECT(stackbuf_append(NULL, "x") == STACKBUF_INVAL);
    EXPECT(stackbuf_append(buf, NULL) == STACKBUF_INVAL);
    EXPECT(stackbuf_used(buf) == 0);
    EXPECT(stackbuf_mark(NULL, &marker) == STACKBUF_INVAL);
    EXPECT(stackbuf_mark(buf, NULL) == STACKBUF_INVAL);
    EXPECT(stackbuf_unmark(buf, NULL) == STACKBUF_INVAL);
    EXPECT(stackbuf_unmark(buf, &marker) == STACKBUF_INVAL);
    EXPECT(stackbuf_finalise(NULL) == STACKBUF_INVAL);
    EXPECT(stackbuf_reset(NULL) == STACKBUF_INVAL);
    EXPECT(stackbuf_destroy(&buf) == STACKBUF_OK);
}

static void run_test(const char *name, void (*fn)(void))
{
    int before = failures;

    current_test = name;
    fn();
    tests_run++;
    if (failures == before) {
        printf("tests/test_stackbuf.c::%s PASSED\n", name);
    } else {
        tests_failed++;
        printf("tests/test_stackbuf.c::%s FAILED\n", name);
    }
    fflush(stdout);
}

int main(void)
{
    int passed;

    run_test("test_version_string_is_semver", test_version_string_is_semver);
    run_test("test_empty_buffer", test_empty_buffer);
    run_test("test_append_concatenates", test_append_concatenates);
    run_test("test_finalise_starts_a_new_string", test_finalise_starts_a_new_string);
    run_test("test_nested_mark_rewinds_inner_text", test_nested_mark_rewinds_inner_text);
    run_test("test_overflow_leaves_the_string_unchanged",
             test_overflow_leaves_the_string_unchanged);
    run_test("test_capacity_one_holds_only_an_empty_string",
             test_capacity_one_holds_only_an_empty_string);
    run_test("test_buffers_are_independent", test_buffers_are_independent);
    run_test("test_invalid_arguments", test_invalid_arguments);

    passed = tests_run - tests_failed;
    if (tests_failed != 0) {
        printf("%d failed, %d passed, %d assertion(s) failed\n",
               tests_failed, passed, failures);
        return 1;
    }
    printf("%d passed\n", passed);
    return 0;
}
