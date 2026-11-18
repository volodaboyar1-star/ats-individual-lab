// Юніт-тести алгоритмів пошуку зразка.
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "encoding.h"
#include "search.h"

using namespace strsearch;

static int gFailed = 0, gPassed = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (cond)                                                         \
            ++gPassed;                                                    \
        else {                                                            \
            ++gFailed;                                                    \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                 \
    } while (0)

// Еталон: усі входження через std::string::find (з перекриттями).
static std::vector<std::size_t> reference(const std::string& t, const std::string& p) {
    std::vector<std::size_t> res;
    if (p.empty()) return res;
    for (auto pos = t.find(p); pos != std::string::npos; pos = t.find(p, pos + 1))
        res.push_back(pos);
    return res;
}

static void checkAll(const std::string& t, const std::string& p) {
    const auto ref = reference(t, p);
    CHECK(naiveSearch(t, p).matches == ref);
    CHECK(kmpSearch(t, p).matches == ref);
    CHECK(boyerMooreSearch(t, p).matches == ref);
    CHECK(naiveSearch<false>(t, p).matches == ref);
    CHECK(kmpSearch<false>(t, p).matches == ref);
    CHECK(boyerMooreSearch<false>(t, p).matches == ref);
}

static void testBasic() {
    std::printf("[basic] класичні приклади\n");
    checkAll("ABABDABACDABABCABAB", "ABABCABAB");
    checkAll("HERE IS A SIMPLE EXAMPLE", "EXAMPLE");
    checkAll("abracadabra", "abra");
    checkAll("aaaaaaaaaa", "aa");          // перекриття входжень
    checkAll("aaaaaaaaaa", "aaaaaaaaaa");  // зразок = текст
    checkAll("abc", "abcd");               // зразок довший за текст
    checkAll("abcdef", "xyz");             // немає входжень
    checkAll("abcdef", "f");               // один символ, в кінці
    checkAll("abcdef", "a");               // один символ, на початку
    checkAll("0101101011010110", "01101");

    CHECK((naiveSearch("abc", "").matches.empty()));
    CHECK((kmpSearch("abc", "").matches.empty()));
    CHECK((boyerMooreSearch("abc", "").matches.empty()));
    CHECK((boyerMooreSearch("", "a").matches.empty()));
    CHECK((kmpSearch("aaaa", "aa").matches == std::vector<std::size_t>{0, 1, 2}));
}

static void testPreprocessing() {
    std::printf("[preprocessing] префікс-функція, таблиці Боєра-Мура\n");
    CHECK((prefixFunction("ABABCABAB") == std::vector<std::size_t>{0, 0, 1, 2, 0, 1, 2, 3, 4}));
    CHECK((prefixFunction("aaaa") == std::vector<std::size_t>{0, 1, 2, 3}));

    const auto last = badCharTable("EXAMPLE");
    CHECK(last['E'] == 6);
    CHECK(last['X'] == 1);
    CHECK(last['P'] == 4);
    CHECK(last['Z'] == -1);

    // Відомий приклад: для "ABBABAB" зсуви хорошого суфікса {5,5,5,5,2,5,4,1}
    // (shift[j] для розбіжності на позиції j-1, shift[m] = 1).
    CHECK((goodSuffixTable("ABBABAB") == std::vector<std::size_t>{5, 5, 5, 5, 2, 5, 4, 1}));
    // "aaaa": сильна евристика не зсуває на позицію, де знову стояла б та сама "a" навпроти
    // символу, що вже дав розбіжність, тому зсуви зростають.
    CHECK((goodSuffixTable("aaaa") == std::vector<std::size_t>{1, 1, 2, 3, 4}));
}

static void testComparisons() {
    std::printf("[comparisons] теоретичні межі кількості порівнянь\n");
    // Наївний: найгірший випадок text = a^n, pattern = a^(m-1)b -> (n-m+1)*m порівнянь.
    const std::string t(1000, 'a');
    const std::string p = std::string(9, 'a') + "b";
    CHECK(naiveSearch(t, p).comparisons == (1000 - 10 + 1) * 10ULL);
    // КМП: не більше 2n порівнянь під час пошуку і 2m при препроцесингу.
    const auto k = kmpSearch(t, p);
    CHECK(k.comparisons <= 2 * t.size());
    CHECK(k.preprocessing <= 2 * p.size());
    // Боєр-Мур на тексті без символів зразка робить ~n/m порівнянь.
    const std::string t2(10000, 'x');
    const auto b = boyerMooreSearch(t2, "abcdefghij");
    CHECK(b.comparisons == 1000);

    std::mt19937 rng(7);
    for (int it = 0; it < 200; ++it) {
        std::string tt(2000, ' '), pp(1 + rng() % 30, ' ');
        for (auto& c : tt) c = static_cast<char>('a' + rng() % 2);
        for (auto& c : pp) c = static_cast<char>('a' + rng() % 2);
        CHECK(kmpSearch(tt, pp).comparisons <= 2 * tt.size());
    }
}

static void testRandom() {
    std::printf("[random] звірка з std::string::find на випадкових даних\n");
    std::mt19937 rng(12345);
    for (int sigma : {1, 2, 3, 4, 26, 200}) {
        for (int it = 0; it < 300; ++it) {
            const std::size_t n = rng() % 300, m = 1 + rng() % 12;
            std::string t(n, ' '), p(m, ' ');
            for (auto& c : t) c = static_cast<char>(0x30 + rng() % sigma);
            for (auto& c : p) c = static_cast<char>(0x30 + rng() % sigma);
            // інколи беремо зразок прямо з тексту, щоб гарантувати входження
            if (n >= m && it % 3 == 0) p = t.substr(rng() % (n - m + 1), m);
            checkAll(t, p);
        }
    }
}

static void testUkrainian() {
    std::printf("[ukrainian] український текст у cp1251\n");
    const std::string t =
        utf8ToCp1251("Ґанок, їжак і єнот: «Привіт!» — сказав пʼятий кіт. Їжак пішов.");
    CHECK(t.size() == 62);  // один символ = один байт
    checkAll(t, utf8ToCp1251("їжак"));
    checkAll(t, utf8ToCp1251("Їжак"));
    checkAll(t, utf8ToCp1251("кіт"));
    checkAll(t, utf8ToCp1251("«Привіт!»"));
    CHECK(kmpSearch(t, utf8ToCp1251("кіт")).matches.size() == 1);
    CHECK(cp1251ToUtf8(utf8ToCp1251("Ґанок їжак Єнот і «»—")) == "Ґанок їжак Єнот і «»—");
    CHECK(utf8ToCp1251("па́м'ять") == utf8ToCp1251("пам'ять"));  // наголос відкидається
}

int main() {
    testBasic();
    testPreprocessing();
    testComparisons();
    testRandom();
    testUkrainian();
    std::printf("\nПройдено перевірок: %d, провалено: %d\n", gPassed, gFailed);
    return gFailed == 0 ? 0 : 1;
}
