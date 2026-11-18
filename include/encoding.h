#pragma once
// Перетворення UTF-8 <-> Windows-1251.
// Український текст переводиться в однобайтове кодування, щоб один символ = один байт:
// тоді довжина зразка вимірюється в літерах, а таблиця поганого символу має 256 елементів.
#include <string>
#include <string_view>

namespace strsearch {

std::string utf8ToCp1251(std::string_view utf8);  // невідомі символи -> '?'
std::string cp1251ToUtf8(std::string_view cp1251);
bool isLetterCp1251(unsigned char c);  // латиниця, кирилиця, апостроф

}  // namespace strsearch
