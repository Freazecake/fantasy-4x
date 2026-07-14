// tests/test_framework.hpp
//
// Минимальный header-only фреймворк для консольных тестов core/.
// Без внешних зависимостей (GTest и т.п. намеренно не тянем — см. правило
// "не тащить тяжёлые библиотеки без необходимости" из инструкций проекта).
//
// Использование:
//
//   #include "test_framework.hpp"
//
//   TEST_CASE("Race/loadFromJson читает модификаторы")
//   {
//       json obj = ...;
//       Race r = Race::loadFromJson(obj);
//       CHECK(r.getName() == "Люди");
//       CHECK_NEAR(r.getModifiers().trade, 1.3f, 0.001f);
//   }
//
// Все TEST_CASE во всех .cpp файлах регистрируются автоматически при старте
// программы (статические объекты). main() просто вызывает testfw::runAll().

#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace testfw {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

// Счётчик провалов внутри ТЕКУЩЕГО теста. runAll() сверяет его до/после
// вызова каждого теста, поэтому один CHECK() не останавливает тест —
// остальные CHECK() в этом же TEST_CASE всё равно выполнятся и покажут
// все проблемы разом, а не только первую.
inline int& failuresInCurrentTest() {
    static int count = 0;
    return count;
}

inline void reportFailure(const std::string& expr, const std::string& file, int line) {
    ++failuresInCurrentTest();
    std::cout << "        [FAIL] " << expr;
    if (!file.empty()) {
        std::cout << "  (" << file << ":" << line << ")";
    }
    std::cout << "\n";
}

inline int runAll() {
    int total = 0;
    int failed = 0;

    for (auto& t : registry()) {
        ++total;
        failuresInCurrentTest() = 0;

        std::cout << "[ RUN  ] " << t.name << "\n";
        try {
            t.fn();
        } catch (const std::exception& e) {
            reportFailure(std::string("неожиданное исключение: ") + e.what(), "", 0);
        } catch (...) {
            reportFailure("неожиданное исключение неизвестного типа", "", 0);
        }

        if (failuresInCurrentTest() == 0) {
            std::cout << "[  OK  ] " << t.name << "\n";
        } else {
            ++failed;
            std::cout << "[ FAIL ] " << t.name << "\n";
        }
    }

    std::cout << "\n==================================================\n";
    std::cout << "Всего тестов: " << total << ", провалено: " << failed << "\n";
    std::cout << "==================================================\n";

    return failed == 0 ? 0 : 1;
}

}  // namespace testfw

#define TESTFW_CONCAT_(a, b) a##b
#define TESTFW_CONCAT(a, b) TESTFW_CONCAT_(a, b)

// TEST_CASE("описание") { ...тело... }
// Регистрирует тест-функцию через статический объект-регистратор,
// вызывается автоматически до входа в main().
#define TEST_CASE(name)                                                        \
    static void TESTFW_CONCAT(testfw_fn_, __LINE__)();                        \
    static testfw::Registrar TESTFW_CONCAT(testfw_reg_, __LINE__)(             \
        name, TESTFW_CONCAT(testfw_fn_, __LINE__));                            \
    static void TESTFW_CONCAT(testfw_fn_, __LINE__)()

// Проверка условия. При провале печатает выражение и продолжает тест
// (не бросает исключение и не прерывает выполнение остальных CHECK).
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            testfw::reportFailure(#cond, __FILE__, __LINE__);                  \
        }                                                                      \
    } while (false)

// Проверка приблизительного равенства float/double.
#define CHECK_NEAR(a, b, eps)                                                  \
    do {                                                                       \
        if (std::fabs((double)(a) - (double)(b)) > (double)(eps)) {            \
            std::ostringstream oss;                                            \
            oss << #a " ~= " #b " (получено " << (a) << ", ожидалось " << (b)  \
                << ")";                                                        \
            testfw::reportFailure(oss.str(), __FILE__, __LINE__);              \
        }                                                                      \
    } while (false)

// Проверка, что выражение бросает исключение (любое).
#define CHECK_THROWS(expr)                                                     \
    do {                                                                       \
        bool threw = false;                                                    \
        try {                                                                  \
            (void)(expr);                                                      \
        } catch (...) {                                                        \
            threw = true;                                                      \
        }                                                                      \
        if (!threw) {                                                          \
            testfw::reportFailure("ожидалось исключение: " #expr, __FILE__,    \
                                   __LINE__);                                  \
        }                                                                      \
    } while (false)

// Проверка, что выражение НЕ бросает исключение.
#define CHECK_NOTHROW(expr)                                                    \
    do {                                                                       \
        try {                                                                  \
            (void)(expr);                                                      \
        } catch (const std::exception& e) {                                    \
            testfw::reportFailure(                                             \
                std::string("неожиданное исключение: " #expr " -> ") + e.what(), \
                __FILE__, __LINE__);                                           \
        } catch (...) {                                                        \
            testfw::reportFailure("неожиданное исключение: " #expr, __FILE__,  \
                                   __LINE__);                                  \
        }                                                                      \
    } while (false)
