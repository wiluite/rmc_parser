#pragma once

namespace serial
{
    template <class M>
    class machine_memento
    {
        friend class machine;
        typename M::const_iterator b;
        typename M::const_iterator e;
    public:
        explicit machine_memento (M const & m) : b(m.begin()), e(m.end()) {}
    };
}
