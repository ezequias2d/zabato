#pragma once

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct test_runner
{
    int m_passed               = 0;
    int m_failed               = 0;
    const char *m_current_test = nullptr;
};

inline test_runner &get_runner()
{
    static test_runner runner;
    return runner;
}

#define TEST(name)                                                             \
    void test_##name();                                                        \
    struct test_reg_##name                                                     \
    {                                                                          \
        test_reg_##name()                                                      \
        {                                                                      \
            get_runner().m_current_test = #name;                               \
            test_##name();                                                     \
        }                                                                      \
    } test_instance_##name;                                                    \
    void test_##name()

#define ASSERT_TRUE(expr)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(expr))                                                           \
        {                                                                      \
            get_runner().m_failed++;                                           \
            std::fprintf(stderr,                                               \
                         "FAIL: %s - %s:%d: %s\n",                             \
                         get_runner().m_current_test,                          \
                         __FILE__,                                             \
                         __LINE__,                                             \
                         #expr);                                               \
            return;                                                            \
        }                                                                      \
        get_runner().m_passed++;                                               \
    } while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NE(a, b) ASSERT_TRUE((a) != (b))
#define ASSERT_LT(a, b) ASSERT_TRUE((a) < (b))
#define ASSERT_GT(a, b) ASSERT_TRUE((a) > (b))
#define ASSERT_LE(a, b) ASSERT_TRUE((a) <= (b))
#define ASSERT_GE(a, b) ASSERT_TRUE((a) >= (b))

#define ASSERT_NEAR(a, b, eps) ASSERT_TRUE(abs((a) - (b)) < (eps))

#define RUN_TESTS()                                                            \
    int main()                                                                 \
    {                                                                          \
        std::printf("Running tests...\n");                                     \
        std::printf("Passed: %d, Failed: %d\n",                                \
                    get_runner().m_passed,                                     \
                    get_runner().m_failed);                                    \
        return get_runner().m_failed > 0 ? 1 : 0;                              \
    }
