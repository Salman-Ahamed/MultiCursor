#pragma once
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#define TEST(suite, name) \
    static void MiniTest_##suite##_##name(); \
    namespace { \
        struct MiniTestReg_##suite##_##name { \
            MiniTestReg_##suite##_##name() { \
                MiniTest::Instance().Register(#suite "." #name, MiniTest_##suite##_##name); \
            } \
        } MiniTestReg_##suite##_##name##_inst; \
    } \
    static void MiniTest_##suite##_##name()

#define EXPECT_TRUE(expr) do { \
    if (!(expr)) { \
        MiniTest::Instance().Fail(__FILE__, __LINE__, "EXPECT_TRUE(" #expr ") failed"); \
        return; \
    } \
} while(0)

#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))

#define EXPECT_EQ(a, b) do { \
    auto _a = (a); auto _b = (b); \
    if (!(_a == _b)) { \
        MiniTest::Instance().Fail(__FILE__, __LINE__, "EXPECT_EQ(" #a ", " #b ") failed"); \
        return; \
    } \
} while(0)

#define EXPECT_NE(a, b) do { \
    auto _a = (a); auto _b = (b); \
    if (!(_a != _b)) { \
        MiniTest::Instance().Fail(__FILE__, __LINE__, "EXPECT_NE(" #a ", " #b ") failed"); \
        return; \
    } \
} while(0)

#define EXPECT_LT(a, b) do { \
    auto _a = (a); auto _b = (b); \
    if (!(_a < _b)) { \
        MiniTest::FailMsg(__FILE__, __LINE__, "EXPECT_LT(" #a ", " #b ") failed"); \
        return; \
    } \
} while(0)

#define EXPECT_FLOAT_EQ(a, b) do { \
    float _a = (float)(a); float _b = (float)(b); \
    float _diff = _a - _b; \
    if (_diff < 0) _diff = -_diff; \
    if (_diff > 0.0001f) { \
        MiniTest::FailMsg(__FILE__, __LINE__, "EXPECT_FLOAT_EQ(" #a ", " #b ") failed"); \
        return; \
    } \
} while(0)

struct MiniTest {
    using TestFunc = std::function<void()>;

    static MiniTest& Instance() {
        static MiniTest inst;
        return inst;
    }

    void Register(const char* name, TestFunc fn) {
        m_tests.emplace_back(name, fn);
    }

    static void FailMsg(const char* file, int line, const char* msg) {
        std::fprintf(stderr, "  FAILED at %s:%d: %s\n", file, line, msg);
    }

    void Fail(const char* file, int line, const char* msg) {
        m_failed = true;
        FailMsg(file, line, msg);
    }

    int RunAll() {
        int passed = 0, failed = 0;
        for (auto& [name, fn] : m_tests) {
            m_failed = false;
            std::printf("[ RUN      ] %s\n", name.c_str());
            fn();
            if (m_failed) {
                std::printf("[  FAILED  ] %s\n", name.c_str());
                failed++;
            } else {
                std::printf("[       OK ] %s\n", name.c_str());
                passed++;
            }
        }
        std::printf("===================================\n");
        std::printf("%d tests, %d passed, %d failed\n", passed + failed, passed, failed);
        return failed > 0 ? 1 : 0;
    }

private:
    std::vector<std::pair<std::string, TestFunc>> m_tests;
    bool m_failed = false;
};

inline int RunAllTests() {
    return MiniTest::Instance().RunAll();
}
