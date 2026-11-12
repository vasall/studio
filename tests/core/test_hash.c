#include "unity.h"

#include <core/hash.h>

/******************************************************************************
 *
 * 				TEST CASES
 *
 ******************************************************************************/

extern void test_known_values(void);
extern void test_str_matches_bytes(void);
extern void test_seed_changes_result(void);
extern void test_chaining(void);
extern void test_null_returns_seed(void);

/******************************************************************************
 *
 * 				TEST RUNNER
 *
 ******************************************************************************/

extern void setUp(void) {}
extern void tearDown(void) {}

int main(void)
{
        UNITY_BEGIN();

        RUN_TEST(test_known_values);
        RUN_TEST(test_str_matches_bytes);
        RUN_TEST(test_seed_changes_result);
        RUN_TEST(test_chaining);
        RUN_TEST(test_null_returns_seed);

        return UNITY_END();
}

/******************************************************************************
 *
 * 				HELPERS
 *
 ******************************************************************************/

static u64 u64_of(u32 hi, u32 lo)
{
        return ((u64)hi << 32) | (u64)lo;
}

/******************************************************************************
 *
 * 				TEST FUNCTIONS
 *
 ******************************************************************************/

/* Reference values of FNV-1a 64 */
extern void test_known_values(void)
{
        TEST_ASSERT_TRUE(hash_str("", HASH_SEED) == HASH_SEED);
        TEST_ASSERT_TRUE(hash_str("a", HASH_SEED) ==
                        u64_of(0xaf63dc4cUL, 0x8601ec8cUL));
        TEST_ASSERT_TRUE(hash_str("foobar", HASH_SEED) ==
                        u64_of(0x85944171UL, 0xf73967e8UL));
}

extern void test_str_matches_bytes(void)
{
        TEST_ASSERT_TRUE(hash_str("tab", HASH_SEED) ==
                        hash_bytes("tab", 3, HASH_SEED));
}

extern void test_seed_changes_result(void)
{
        TEST_ASSERT_TRUE(hash_str("tab", 1) != hash_str("tab", 2));
}

extern void test_chaining(void)
{
        /* Hashing in two parts gives the same as hashing all at once */
        TEST_ASSERT_TRUE(hash_str("bar", hash_str("foo", HASH_SEED)) ==
                        hash_str("foobar", HASH_SEED));
}

extern void test_null_returns_seed(void)
{
        TEST_ASSERT_TRUE(hash_str(NULL, 7) == 7);
        TEST_ASSERT_TRUE(hash_bytes(NULL, 5, 7) == 7);
}
