//
// Created by user on 13.01.2020.
//

#ifndef RMC_PARSER_STATES_H
#define RMC_PARSER_STATES_H
#include <memory>
#include <iostream>

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
            auto const find_result = std::find(std::begin(machine_), machine_.end(), '$');
            if (find_result == machine_.end())
            {
                machine_.align();
                return false;
            } else
            {
                machine_.align(find_result);
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
                std::array<char, 3> rmc_seq {'R', 'M', 'C'};
                auto _ = std::search (machine_.begin(), machine_.end(), rmc_seq.begin(), rmc_seq.end());
                if ((_ == machine_.end()) || (machine_.distance(machine_.get_start(), _) != 2))
                {
                    machine_.align();
                    machine_.set_state(machine_.get_parse_$_state());
                    return false;
                } else
                {
                    machine_.align(++++_);
                    machine_.set_state(machine_.get_parse_crlf_state());
                    return true;
                }
            }
            return false;
        }
    };

    template <typename MACHINE>
    class ParseCrlfState final : public parent_state<MACHINE>
    {
        using parent_class_type = parent_state<MACHINE>;
        using parent_class_type::machine_;
    public:
        explicit ParseCrlfState (typename parent_class_type::machine_type machine) : parent_class_type(machine) {}

        bool parse() noexcept override
        {
//            std::cout << "size " << machine_.size() << std::endl;
//            std::cout << this << std::endl;
            std::array<char, 2> crlf_seq {'\x0D', '\x0A'};
            auto _ = std::search (machine_.begin(), machine_.end(), crlf_seq.begin(), crlf_seq.end());
            auto inc_iter = std::begin(machine_);
            if (_ == machine_.end())
            {
                while (machine_.size() > 1)
                {
                    machine_.strict_align(++inc_iter);

                    //machine_.align();
                }
                return false;
            }
            return false;
        }
    };
}

#endif //RMC_PARSER_STATES_H
