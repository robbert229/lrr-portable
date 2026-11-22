// SPDX-License-Identifier: LGPL-3.0-or-later
/* lrr_test.h — the assertions the unit tests share. Header-only C++17, no
 * dependency beyond the standard library.
 *
 *   #include "lrr_test.h"
 *
 *   int main(void)
 *   {
 *       CHECK(p != NULL, "the record is allocated");
 *       CHECK_EQ(record.count, 5u);
 *       CHECK_NEAR(r.x, 1.2f, 1e-6);
 *       CHECK_STREQ(item.name, "item3");
 *       return lrr_test_result("reward");
 *   }
 *
 * A failed check prints "FAIL: file:line: ..." with what it compared, counts
 * the failure and carries on. Each check also returns whether it passed.
 * CHECK_EQ compares with ==, so write an unsigned expected value as 5u, as
 * the comparison would be written by hand.
 * lrr_test_result prints a summary and returns 1 if any check failed, else
 * 0: not the count itself, which ctest would read as a skip at 77
 * (SKIP_RETURN_CODE) and as a pass at 256. */
#ifndef LRR_TEST_H
#define LRR_TEST_H

#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace lrr_test {

inline int failures = 0;

inline void fail(const char* file, int line, const std::string& message)
{
    std::printf("FAIL: %s:%d: %s\n", file, line, message.c_str());
    std::fflush(stdout);
    ++failures;
}

template <typename T, typename = void>
struct printable : std::false_type {};
template <typename T>
struct printable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>>
    : std::true_type {};

/* A value as a failure message shows it: characters as numbers, floats
 * with every digit that tells two apart, enums as their number, C strings
 * quoted. */
template <typename T>
std::string show(const T& value)
{
    std::ostringstream out;
    if constexpr (std::is_same_v<T, bool>) {
        out << (value ? "true" : "false");
    } else if constexpr (std::is_enum_v<T>) {
        out << +static_cast<std::underlying_type_t<T>>(value);
    } else if constexpr (std::is_integral_v<T>) {
        out << +value;
    } else if constexpr (std::is_floating_point_v<T>) {
        out.precision(9);
        out << value;
    } else if constexpr (std::is_same_v<std::decay_t<T>, char*> ||
                         std::is_same_v<std::decay_t<T>, const char*>) {
        if (value == nullptr) out << "NULL";
        else out << '"' << value << '"';
    } else if constexpr (std::is_pointer_v<T> || std::is_null_pointer_v<T>) {
        out << static_cast<const void*>(value);
    } else if constexpr (printable<T>::value) {
        out << value;
    } else {
        out << "(" << sizeof(T) << "-byte value)";
    }
    return out.str();
}

inline bool check(bool cond, const char* what, const char* file, int line)
{
    if (!cond) fail(file, line, what);
    return cond;
}

template <typename A, typename B>
bool check_eq(const A& actual, const B& expected, const char* actual_text,
              const char* expected_text, const char* file, int line)
{
    if (actual == expected) return true;
    fail(file, line,
         std::string(actual_text) + " == " + expected_text + ": got " + show(actual) +
             ", expected " + show(expected));
    return false;
}

template <typename A, typename B, typename E>
bool check_near(const A& actual, const B& expected, const E& eps, const char* actual_text,
                const char* expected_text, const char* file, int line)
{
    const double a = static_cast<double>(actual);
    const double b = static_cast<double>(expected);
    const double e = static_cast<double>(eps);
    /* Written so a NaN on either side fails. */
    if (std::fabs(a - b) <= e) return true;
    fail(file, line,
         std::string(actual_text) + " near " + expected_text + ": got " + show(actual) +
             ", expected " + show(expected) + " within " + show(eps));
    return false;
}

inline bool check_streq(const char* actual, const char* expected, const char* actual_text,
                        const char* expected_text, const char* file, int line)
{
    if (actual != nullptr && expected != nullptr && std::strcmp(actual, expected) == 0)
        return true;
    if (actual == nullptr && expected == nullptr) return true;
    fail(file, line,
         std::string(actual_text) + " equals " + expected_text + ": got " + show(actual) +
             ", expected " + show(expected));
    return false;
}

} // namespace lrr_test

/* CHECK(cond, what): the call style the tests already use. */
#define CHECK(cond, what) ::lrr_test::check(static_cast<bool>(cond), (what), __FILE__, __LINE__)
#define CHECK_EQ(actual, expected) \
    ::lrr_test::check_eq((actual), (expected), #actual, #expected, __FILE__, __LINE__)
#define CHECK_NEAR(actual, expected, eps) \
    ::lrr_test::check_near((actual), (expected), (eps), #actual, #expected, __FILE__, __LINE__)
#define CHECK_STREQ(actual, expected) \
    ::lrr_test::check_streq((actual), (expected), #actual, #expected, __FILE__, __LINE__)

/* The failure count so far. */
inline int lrr_test_failures(void)
{
    return ::lrr_test::failures;
}

/* main's return: prints "<name>: all checks passed" or "<name>: N
 * failure(s)", and returns 0 or 1. */
inline int lrr_test_result(const char* name = "test")
{
    if (::lrr_test::failures == 0) {
        std::printf("%s: all checks passed\n", name);
        return 0;
    }
    std::printf("%s: %d failure(s)\n", name, ::lrr_test::failures);
    return 1;
}

#endif
