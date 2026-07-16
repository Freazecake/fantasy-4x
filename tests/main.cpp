// tests/main.cpp
//
// Консольный тест-раннер для core/ (неделя 2, день 7 роадмапа).
// Ничего не знает про конкретные тесты — они регистрируют себя сами
// через TEST_CASE(...) в своих .cpp файлах (см. test_framework.hpp).

#include "test_framework.hpp"

int main() {
    return testfw::runAll();
}
