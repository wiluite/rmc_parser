#include <iostream>
#include <ring_iter.h>
#include <cassert>
#include <cstddef>


template<typename... Ts>
std::array<std::byte, sizeof...(Ts)> make_bytes(Ts && ... args) noexcept
{
    return {std::byte(std::forward<Ts>(args))...};
}

int main()
{
    using namespace funny_it;
}


