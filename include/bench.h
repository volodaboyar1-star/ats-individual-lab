#pragma once
// Експериментальне порівняння алгоритмів пошуку.
#include <string>

namespace strsearch {

struct BenchConfig {
    std::string naturalTextPath = "data/uk_text.txt";  // UTF-8, від 1 МБ
    std::string outDir = "results";                    // куди писати CSV
    std::size_t randomTextSize = 1'000'000;            // розмір випадкових текстів
    int patternsPerLength = 50;                        // зразків на кожну довжину
    int timingRepeats = 3;                             // повторів заміру часу (береться мінімум)
    unsigned seed = 2026;
};

// Запускає три експерименти:
//  1) випадковий бінарний зразок у випадковому бінарному тексті, довжини 5..50;
//  2) випадкове слово в українському тексті, довжини 5..50;
//  3) вплив розміру алфавіту (випадкові тексти з алфавітом 2..128 символів).
// Повертає 0 при успіху.
int runBenchmarks(const BenchConfig& cfg);

}  // namespace strsearch
