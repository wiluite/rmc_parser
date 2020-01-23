#ifndef RMC_PARSER_STATES_H
#define RMC_PARSER_STATES_H
#include <memory>
#include <iostream>
#include <ring_iter.h>
#include "mach_mem.h"
#include "light_string.h"

namespace serial
{
    class state
    {
    public:
        virtual ~state() = default;
        virtual bool parse() = 0;
        virtual void cleanup() {}
    };

    using state_ptr = std::unique_ptr<state>;

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
        explicit Parse$State (typename parent_class_type::machine_type machine) : parent_class_type(machine) {}

        bool parse() noexcept override
        {
            auto const __ = std::find(std::begin(machine_), machine_.end(), '$');
            if (__ == machine_.end())
            {
                machine_.align();
                return false;
            } else
            {
                machine_.align(__);
                machine_.set_state(machine_.get_parse_rmc_state()); // cleanup call here
                return true;
            }
        }

        void cleanup() noexcept final
        {
            machine_.save_start(machine_.begin());
        }
    };

    template <typename MACHINE>
    class ParseRmcState final : public parent_state<MACHINE>
    {
        using parent_class_type = parent_state<MACHINE>;
        using parent_class_type::machine_;
    public:
        explicit ParseRmcState (typename parent_class_type::machine_type machine) : parent_class_type(machine) {}

        bool parse() noexcept override
        {
            if (machine_.size() >= 5)
            {
                std::array<char, 3> const rmc_seq {'R', 'M', 'C'};
                auto __ = std::search (machine_.begin(), machine_.end(), rmc_seq.begin(), rmc_seq.end());
                if ((__ == machine_.end()) || (machine_.distance(machine_.get_start(), __) != 2))
                {
                    machine_.align();
                    machine_.set_state(machine_.get_parse_$_state());
                } else
                {
                    machine_.align(++++__);
                    machine_.set_state(machine_.get_parse_crlf_state());
                }
                return true;
            }
            return false;
        }
    };

    template <typename MACHINE>
    class ParseCrlfState final : public parent_state<MACHINE>
    {
        using parent_class_type = parent_state<MACHINE>;
        using parent_class_type::machine_;
        const uint8_t max_msg_size = 82;
        uint8_t msg_size = 0;
        bool on_max_msg_size() noexcept
        {
            // code smell... fix me.
            if (msg_size > max_msg_size) {
                machine_.align();
                machine_.set_state(machine_.get_parse_$_state());
                return true;
            }
            return false;
        }
    public:
        explicit ParseCrlfState (typename parent_class_type::machine_type machine) : parent_class_type(machine) {}

        bool parse() noexcept override
        {
            std::array<char, 2> const crlf_seq {'\x0D', '\x0A'};
            auto __ = std::search (machine_.begin(), machine_.end(), crlf_seq.begin(), crlf_seq.end());

            if (__ == machine_.end())
            {
                if (machine_.size() > 1)
                {
                    auto const sz = machine_.size();
                    machine_.strict_align(std::begin(machine_)+(sz-1));
                    msg_size += (sz - 1);
                }
                return on_max_msg_size();
            } else
            {
                machine_.save_stop(__);
                machine_.align(++__);
                machine_.set_state(machine_.get_parse_checksum_state());
                return true;
            }
        }

        void cleanup() noexcept final
        {
            msg_size = 0;
        }
    };

    // caretaker (memento pattern)
    template <typename MACHINE>
    class ParseChecksumState final : public parent_state<MACHINE>
    {
        using parent_class_type = parent_state<MACHINE>;
        using parent_class_type::machine_;
        std::unique_ptr<machine_memento<MACHINE>> mm {nullptr};
        char msg_checksum[3] {0,0,'\0'};

        [[nodiscard]] int calc_cs() const noexcept
        {
            int sum = 0;
            for (auto const & elem : machine_)
            {
                if ('*' == elem)
                    break;
                if (!sum)
                {
                    sum = elem;
                } else
                {
                    sum ^= elem;
                }
            }
            return sum;
        }

    public:
        explicit ParseChecksumState(typename parent_class_type::machine_type machine) : parent_class_type(machine) {}

        bool parse() noexcept final
        {
            mm = machine_.check_point();

            machine_.reset(machine_.get_start(), machine_.get_stop());
            auto const star_it = machine_.begin() + (machine_.size() - 3);

            if (*star_it != '*')
            {
                machine_.set_state(machine_.get_parse_$_state());
                return true;
            }

            std::uninitialized_copy (machine_.begin() + (machine_.size() - 2), machine_.begin() + (machine_.size() - 0), msg_checksum);

            if (calc_cs() == strtol(msg_checksum, nullptr, 16))
            {
                machine_.process(string_type(machine_.begin(), machine_.end()));
            }

            machine_.set_state(machine_.get_parse_$_state());
            return true;
        }

        void cleanup() noexcept final
        {
            machine_.rollback(std::move(mm));
        }

    };

}

#endif //RMC_PARSER_STATES_H
