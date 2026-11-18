#pragma once
// Алгоритми пошуку зразка в текстовому рядку:
//   наївний, Кнута-Морріса-Пратта (КМП), Боєра-Мура (поганий символ + хороший суфікс).
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace strsearch {

struct SearchResult {
    std::vector<std::size_t> matches;  // позиції входжень зразка (з 0)
    std::uint64_t comparisons = 0;     // порівняння символів "текст - зразок" під час пошуку
    std::uint64_t preprocessing = 0;   // порівняння символів під час попередньої обробки зразка
};

// Count = true  -> рахуються порівняння символів (для аналізу ефективності);
// Count = false -> "чиста" версія без лічильників (для заміру часу).
template <bool Count = true>
SearchResult naiveSearch(std::string_view text, std::string_view pattern);

template <bool Count = true>
SearchResult kmpSearch(std::string_view text, std::string_view pattern);

template <bool Count = true>
SearchResult boyerMooreSearch(std::string_view text, std::string_view pattern);

// Допоміжні функції попередньої обробки (відкриті для тестів і звіту).
// Префікс-функція: pi[i] = довжина найдовшого власного префікса p[0..i], що є його суфіксом.
std::vector<std::size_t> prefixFunction(std::string_view p, std::uint64_t* comparisons = nullptr);
// Евристика поганого символу: last[c] = остання позиція символу c у зразку або -1.
std::vector<std::ptrdiff_t> badCharTable(std::string_view p);
// Евристика хорошого суфікса (сильна версія): shift[j] = зсув, якщо розбіжність на позиції j-1.
std::vector<std::size_t> goodSuffixTable(std::string_view p, std::uint64_t* comparisons = nullptr);

}  // namespace strsearch
