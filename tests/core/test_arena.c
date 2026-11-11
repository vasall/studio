#include "unity.h"

#include <string.h>

#include <core/arena.h>

void setUp(void) {}
void tearDown(void) {}

void test_arena_alloc_is_zeroed_and_aligned(void)
{
        Arena a;
        u8 *p;
        u8 *q;

        arena_init(&a, 64);
        p = arena_alloc(&a, 10);
        q = arena_alloc(&a, 100);  /* bigger than a block */
        TEST_ASSERT_NOT_NULL(p);
        TEST_ASSERT_NOT_NULL(q);
        TEST_ASSERT_EQUAL_INT(0, (s32)((size_t)p % 16));
        TEST_ASSERT_EQUAL_INT(0, (s32)((size_t)q % 16));
        TEST_ASSERT_EQUAL_UINT8(0, p[9]);
        TEST_ASSERT_EQUAL_UINT8(0, q[99]);
        arena_deinit(&a);
}

void test_arena_reset_reuses_blocks(void)
{
        Arena a;
        void *p1;
        void *p2;

        arena_init(&a, 1024);
        p1 = arena_alloc(&a, 100);
        arena_reset(&a);
        p2 = arena_alloc(&a, 100);
        TEST_ASSERT_EQUAL_PTR(p1, p2);
        arena_deinit(&a);
}

void test_arena_printf(void)
{
        Arena a;
        char *s;

        arena_init(&a, 64);
        s = arena_printf(&a, "%s %d", "tab", 12);
        TEST_ASSERT_EQUAL_STRING("tab 12", s);
        s = arena_strdup(&a, "hello world", 5);
        TEST_ASSERT_EQUAL_STRING("hello", s);
        arena_deinit(&a);
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_arena_alloc_is_zeroed_and_aligned);
        RUN_TEST(test_arena_reset_reuses_blocks);
        RUN_TEST(test_arena_printf);
        return UNITY_END();
}
