#pragma once

#include <memory>
#include <iostream>
#include <ring_iter.h>

#ifdef _MSC_VER
#pragma warning(disable : 4625)
#pragma warning(disable : 4626)
#pragma warning(disable : 5026)
#pragma warning(disable : 5027)
#pragma warning(disable : 4820)
#endif

namespace serial
{
    class state
    {
    public:
        virtual ~state() = default;
        virtual bool parse() = 0;
        virtual void cleanup() {}
    };

    template <typename MACHINE>
    class parent_state : public state
    {
    protected:
        using machine_type = MACHINE&;
        machine_type machine_;
    public:
        explicit parent_state (machine_type machine) : machine_(machine ) {}
        parent_state(const parent_state&) = delete;
        parent_state& operator=(const parent_state&) = delete;
        parent_state(parent_state&&) = delete;
        parent_state& operator=(parent_state&&) = delete;
    };

    template <typename MACHINE>
    class Parse$State final : public parent_state<MACHINE>
    {
        using parent_class_type = parent_state<MACHINE>;
        using parent_class_type::machine_;
    public:
        explicit Parse$State (decltype(machine_) machine) : parent_class_type(machine) {}

        bool parse() noexcept override
        {
            auto const __ = std::find(std::begin(machine_), machine_.end(), '$');
            if (__ == machine_.end())
            {
                machine_.align();
                return false;
            } else
            {
                machine_.align(__+1);
                machine_.set_state(machine_.get_parse_rmc_state()); // cleanup call here
                return true;
            }
        }

        void cleanup() noexcept final
        {
            machine_.save_start(machine_.begin());
        }
        static char storage[100];
    };
    template <typename M>
    char Parse$State<M>::storage[100];

    template <typename MACHINE>
    class ParseRmcState final : public parent_state<MACHINE>
    {
        using parent_class_type = parent_state<MACHINE>;
        using parent_class_type::machine_;
    public:
        explicit ParseRmcState (decltype(machine_) machine) : parent_class_type(machine) {}

        bool parse() noexcept override
        {
            if (machine_.size() > 4)
            {
                std::array<char, 3> constexpr rmc_seq {'R', 'M', 'C'};
                auto const __ = std::search (machine_.begin(), machine_.end(), rmc_seq.begin(), rmc_seq.end());
                if ((__ == machine_.end()) || (machine_.distance(machine_.get_start(), __) != 2))
                {
                    machine_.align();
                    machine_.set_state(machine_.get_parse_$_state());
                } else
                {
                    machine_.align(__ + sizeof(rmc_seq));
                    machine_.set_state(machine_.get_parse_crlf_state());
                }
                return true;
            }
            return false;
        }
        static char storage[100];
    };
    template <typename M>
    char ParseRmcState<M>::storage[100];


    template <typename MACHINE>
    class ParseCrlfState final : public parent_state<MACHINE>
    {
        using parent_class_type = parent_state<MACHINE>;
        using parent_class_type::machine_;

        static constexpr uint8_t max_msg_size = 82;

        template <class IT>
        void handle_adhesion(IT const & it) const noexcept // handles the absence of cr-lf between two messages
        {
            machine_.reset(machine_.get_start(), it);
            machine_.align(machine_.begin() + (max_msg_size >> 1u));
        }

        uint16_t msg_size = 0;
        template <class IT>
        [[nodiscard]] bool on_max_msg_size(IT const & it) const noexcept
        {
            if (msg_size > max_msg_size)
            {
                handle_adhesion(it);
                machine_.set_state(machine_.get_parse_$_state());
                return true;
            }
            return false;
        }
    public:
        explicit ParseCrlfState (decltype(machine_) machine)  : parent_class_type(machine) {}

        bool parse() noexcept override
        {
            if (machine_.size() > 1)
            {
                std::array<char, 2> constexpr crlf_seq {'\x0D', '\x0A'};
                auto const __ = std::search (machine_.begin(), machine_.end(), crlf_seq.begin(), crlf_seq.end());

                if (__ == machine_.end())
                {
                    auto const sz = machine_.size();
                    machine_.align(std::begin(machine_) + (sz - 1));
                    msg_size += (sz - 1);
                    return on_max_msg_size(machine_.end());
                } else
                {
                    machine_memento<MACHINE> mm(machine_);
                    mm = machine_.check_point();
                    machine_.unchecked_reset(machine_.get_start(), __);
                    msg_size = machine_.size();
                    machine_.unchecked_rollback(mm);

                    if (on_max_msg_size(__ + sizeof(crlf_seq)))
                    {
                        return true;
                    }

                    machine_.save_stop(__);
                    machine_.align(__ + sizeof(crlf_seq));
                    machine_.set_state(machine_.get_parse_checksum_state());
                    return true;
                }
            }
            return false;
        }

        void cleanup() noexcept final
        {
            msg_size = 0;
        }
        static char storage[100];
    };
    template <typename M>
    char ParseCrlfState<M>::storage[100];

    // caretaker (memento pattern)
    template <typename MACHINE>
    class ParseChecksumState final : public parent_state<MACHINE>
    {
        using parent_class_type = parent_state<MACHINE>;
        using parent_class_type::machine_;
        machine_memento<MACHINE> mm;
        char msg_checksum[3] {0,0,'\0'};

        [[nodiscard]] int8_t calc_cs() const noexcept
        {
            // not a range-based loop in order to not checking for '*' on each cycle
            auto sum = *machine_.begin();
            std::for_each(machine_.begin()+1, machine_.begin()+(machine_.size()-3), [&sum](char elem)
                          {
                              sum ^= (uint8_t)elem;
                          }
            );
            return sum;
        }

    public:
        explicit ParseChecksumState(decltype(machine_) machine) : parent_class_type(machine), mm(machine) {}

        bool parse() noexcept final
        {
            mm = machine_.check_point();

            machine_.reset(machine_.get_start(), machine_.get_stop());
            auto const star_it = machine_.begin() + (machine_.size() - 3);

            if ('*' != *star_it)
            {
                machine_.set_state(machine_.get_parse_$_state());
                return true;
            }

            std::uninitialized_copy (machine_.begin() + (machine_.size() - 2), machine_.begin() + (machine_.size() - 0), msg_checksum);

            if (calc_cs() == strtol(msg_checksum, nullptr, 16))
            {
                machine_.process();
            }

            machine_.set_state(machine_.get_parse_$_state());
            return true;
        }

        void cleanup() noexcept final
        {
            machine_.rollback(mm);
        }
        static char storage[100];
    };
    template <typename M>
    char ParseChecksumState<M>::storage[100];

}

