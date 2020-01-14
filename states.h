//
// Created by user on 13.01.2020.
//

#ifndef RMC_PARSER_STATES_H
#define RMC_PARSER_STATES_H
#include <memory>
#include <iostream>
#include <ring_iter.h>

namespace serial
{
    class state
    {
    public:
        virtual ~state() = default;
        virtual bool parse() = 0;
        virtual void cleanup() {}
        void print_this()
        {
            std::cout << (int*)this << std::endl;
        }
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
        const uint8_t max_search_size = 82;
        uint8_t search_size = 0;
        bool on_max_search_size() noexcept
        {
            if (search_size > max_search_size) {
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
                auto inc_iter = std::begin(machine_);
                while (machine_.size() > 1)
                {
                    machine_.strict_align(++inc_iter);
                    ++search_size;
                }
                return on_max_search_size();
            } else
            {
                machine_.save_stop(__);
                machine_.align(++__);
                machine_.set_state(machine_.get_parse_checksum_state());
                std::cout << *machine_.get_start() << std::endl;
//                __try
//                {
//                    //machine_.strict_align(machine_.get_start());
//                    std::cout << "dist " << machine_.distance(machine_.get_start(), __) << std::endl;
//                }
//                catch(funny_it::out_of_bounds & ex)
//                {
//                    std::cout << "funny_it::out_of_bounds\n";
//                }
                return true;
            }
        }

        void cleanup() noexcept final
        {
            search_size = 0;
        }
    };

    template <typename MACHINE>
    class ParseChecksumState final : public parent_state<MACHINE>
    {
        using parent_class_type = parent_state<MACHINE>;
        using parent_class_type::machine_;
    public:
        explicit ParseChecksumState(typename parent_class_type::machine_type machine) : parent_class_type(machine) {}

        bool parse() noexcept override
        {
            return false;
        }

    };

}

#endif //RMC_PARSER_STATES_H
