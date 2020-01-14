//
// Created by user on 13.01.2020.
//

#ifndef RMC_PARSER_MACHINE_H
#define RMC_PARSER_MACHINE_H
#include <ring_iter.h>
#include "states.h"

namespace serial
{
    using namespace funny_it;

    constexpr int buf_sz = 1000;
    class machine : private ring_buffer_sequence<char, buf_sz>
    {
    private:
        static char buffer [buf_sz];
        using parent_class_type = ring_buffer_sequence<char, buf_sz>;
    protected:
        using parent_class_type::fill_data;
    public:
        using parent_class_type::align;
        using parent_class_type::size;
        using parent_class_type::distance;
        using parent_class_type::begin;
        using parent_class_type::end;
    private:
        using class_type = machine;
        using parse_$_state_type = Parse$State<class_type>;
        using parse_rmc_state_type = ParseRmcState<class_type>;
        using parse_crlf_state_type = ParseCrlfState<class_type>;

        friend parse_$_state_type;
        friend parse_rmc_state_type;
        friend parse_crlf_state_type;
        const state_ptr parse_$_state_;
        const state_ptr parse_rmc_state_;
        const state_ptr parse_crlf_state_;
    protected:
        state* current_state {nullptr};

    public:
        machine() : ring_buffer_sequence<char,buf_sz>(buffer)
                , parse_$_state_(std::make_unique<parse_$_state_type>(*this))
                , parse_rmc_state_(std::make_unique<parse_rmc_state_type>(*this))
                , parse_crlf_state_(std::make_unique<parse_crlf_state_type>(*this))
        {
            current_state = parse_$_state_.get();
        }

        [[nodiscard]] bool parse() const noexcept
        {
            return current_state->parse();
        }

        [[nodiscard]] const state_ptr& get_parse_$_state() const noexcept
        {
            return parse_$_state_;
        }

        [[nodiscard]] const state_ptr& get_parse_rmc_state() const noexcept
        {
            return parse_rmc_state_;
        }

        [[nodiscard]] const state_ptr& get_parse_crlf_state() const noexcept
        {
            return parse_crlf_state_;
        }

        void set_state (state_ptr const & state ) noexcept
        {
            assert(state);
            assert(current_state);
            current_state->cleanup();
            current_state = state.get();
        }

    private:
        const_iterator start_iterator = begin();
        void save_start(const_iterator it)
        {
            start_iterator = it;
        }
    public:
        [[nodiscard]] const_iterator get_start() const noexcept
        {
            return start_iterator;
        }

    };
    char machine::buffer[buf_sz] = {};
}
#endif //RMC_PARSER_MACHINE_H
