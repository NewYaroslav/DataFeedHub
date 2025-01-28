#include <iostream>
#include <cassert>
#include <DataFeedHub/src/utils/DynamicBitset.hpp>

/// \brief Print the bitset for debugging
void print(const dfh::utils::DynamicBitset& bitset) {
    for (size_t i = 0; i < bitset.size(); ++i) {
        std::cout << bitset.is_set(i);
    }
    std::cout << std::endl;
}

void test_dynamic_bitset() {
    using dfh::utils::DynamicBitset;

    // Размер 8 бит
    DynamicBitset a(8), b(8);
    a.set(0);
    a.set(2);
    a.set(4);
    b.set(1);
    b.set(2);
    b.set(3);

    // Проверка | (ИЛИ)
    DynamicBitset c = a | b;
    assert(c.is_set(0) == true);
    assert(c.is_set(1) == true);
    assert(c.is_set(2) == true);
    assert(c.is_set(3) == true);
    assert(c.is_set(4) == true);
    assert(c.is_set(5) == false);

    // Проверка & (И)
    DynamicBitset d = a & b;
    assert(d.is_set(0) == false);
    assert(d.is_set(1) == false);
    assert(d.is_set(2) == true);
    assert(d.is_set(3) == false);
    assert(d.is_set(4) == false);

    // Проверка << (сдвиг влево)
    DynamicBitset e = a << 2;
    assert(e.is_set(2) == true);
    assert(e.is_set(4) == true);
    assert(e.is_set(6) == true);
    assert(e.is_set(0) == false);

    // Проверка >> (сдвиг вправо)
    DynamicBitset f = b >> 1;
    assert(f.is_set(0) == true);
    assert(f.is_set(1) == true);
    assert(f.is_set(2) == true);
    assert(f.is_set(3) == false);

    // Размер 64 бит
    DynamicBitset g(64);
    g.set(0);
    g.set(63);
    DynamicBitset h(64);
    h.set(1);
    h.set(62);

    // Проверка | (ИЛИ)
    DynamicBitset i = g | h;
    assert(i.is_set(0) == true);
    assert(i.is_set(1) == true);
    assert(i.is_set(62) == true);
    assert(i.is_set(63) == true);

    // Проверка & (И)
    DynamicBitset j = g & h;
    assert(j.is_set(0) == false);
    assert(j.is_set(1) == false);
    assert(j.is_set(62) == false);
    assert(j.is_set(63) == false);

    // Размер больше 64, но меньше 128 бит (96 бит)
    DynamicBitset k(96);
    k.set(0);
    k.set(64);
    k.set(95);
    DynamicBitset l(96);
    l.set(1);
    l.set(65);
    l.set(94);

    // Проверка | (ИЛИ)
    DynamicBitset m = k | l;
    assert(m.is_set(0) == true);
    assert(m.is_set(1) == true);
    assert(m.is_set(64) == true);
    assert(m.is_set(65) == true);
    assert(m.is_set(94) == true);
    assert(m.is_set(95) == true);

    // Проверка & (И)
    DynamicBitset n = k & l;
    assert(n.is_set(0) == false);
    assert(n.is_set(1) == false);
    assert(n.is_set(64) == false);
    assert(n.is_set(65) == false);
    assert(n.is_set(94) == false);
    assert(n.is_set(95) == false);

    // Размер больше 128 бит
    DynamicBitset o(130);
    o.set(0);
    o.set(64);
    o.set(129);
    DynamicBitset p(130);
    p.set(1);
    p.set(65);
    p.set(128);

    // Проверка | (ИЛИ)
    DynamicBitset q = o | p;
    assert(q.is_set(0) == true);
    assert(q.is_set(1) == true);
    assert(q.is_set(64) == true);
    assert(q.is_set(65) == true);
    assert(q.is_set(128) == true);
    assert(q.is_set(129) == true);

    // Проверка & (И)
    DynamicBitset r = o & p;
    assert(r.is_set(0) == false);
    assert(r.is_set(1) == false);
    assert(r.is_set(64) == false);
    assert(r.is_set(65) == false);
    assert(r.is_set(128) == false);
    assert(r.is_set(129) == false);

    std::cout << "All tests passed successfully!" << std::endl;
}

int main() {
    test_dynamic_bitset();
    return 0;
}
