#include <iostream>
#include <ring_iter.h>
#include <bit_iter.h>
#include <cassert>


template<typename... Ts>
std::array<std::byte, sizeof...(Ts)> make_bytes(Ts && ... args) noexcept
{
    return {std::byte(std::forward<Ts>(args))...};
}

int main()
{
    using namespace funny_it;
    std::cout << "wwww\n";
    bit_sequence seq {make_bytes(0x0A, 0x0B, 0x0C)}; // 00001010  00001011  00001100 (7 bits by 1, 17 bits by 0)

    assert (std::count(std::begin(seq), std::end(seq),std::byte{1}) == 7);
}


