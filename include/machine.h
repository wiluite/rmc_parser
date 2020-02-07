#pragma once

#include <ring_iter.h>
#include <states.h>
#include <parser.h>

namespace serial
{
    using namespace funny_it;

    template <size_t bs, class RMC_Callback>
    class machine : private ring_buffer_sequence<char, bs>
    {
    private:
        static char buffer [bs];
        using parent_class_type = ring_buffer_sequence<char, bs>;
    protected:
        using parent_class_type::fill_data;
    public:
        using parent_class_type::align;
        using parent_class_type::size;
        using parent_class_type::distance;
        using parent_class_type::begin;
        using parent_class_type::end;
        using parent_class_type::head;
        using parent_class_type::tail;
        using const_iterator = typename parent_class_type::const_iterator;
    private:
        using class_type = machine;
        using parse_$_state_type = Parse$State<class_type>;
        using parse_rmc_state_type = ParseRmcState<class_type>;
        using parse_crlf_state_type = ParseCrlfState<class_type>;
        using parse_checksum_state_type = ParseChecksumState<class_type>;

        friend parse_$_state_type;
        friend parse_crlf_state_type;
        friend parse_checksum_state_type;

        const state_ptr parse_$_state_;
        const state_ptr parse_rmc_state_;
        const state_ptr parse_crlf_state_;
        const state_ptr parse_checksum_state_;
    protected:
        state* current_state {nullptr};

    public:
        machine() : ring_buffer_sequence<char, bs>(buffer)
                , parse_$_state_(std::make_unique<parse_$_state_type>(*this))
                , parse_rmc_state_(std::make_unique<parse_rmc_state_type>(*this))
                , parse_crlf_state_(std::make_unique<parse_crlf_state_type>(*this))
                , parse_checksum_state_(std::make_unique<parse_checksum_state_type>(*this))
        {
            current_state = parse_$_state_.get();
        }

        [[nodiscard]] constexpr bool parse() const noexcept
        {
            return current_state->parse();
        }

        [[nodiscard]] constexpr const state_ptr& get_parse_$_state() const noexcept
        {
            return parse_$_state_;
        }

        [[nodiscard]] constexpr const state_ptr& get_parse_rmc_state() const noexcept
        {
            return parse_rmc_state_;
        }

        [[nodiscard]] constexpr const state_ptr& get_parse_crlf_state() const noexcept
        {
            return parse_crlf_state_;
        }

        [[nodiscard]] constexpr const state_ptr& get_parse_checksum_state() const noexcept
        {
            return parse_checksum_state_;
        }

        constexpr void set_state (state_ptr const & state ) noexcept
        {
            assert(state);
            assert(current_state);
            current_state->cleanup();
            current_state = state.get();
        }

    private:
        const_iterator start_iterator = begin();
        const_iterator stop_iterator = begin();
        constexpr void save_start(const_iterator it) noexcept
        {
            start_iterator = it;
        }

        constexpr void save_stop(const_iterator it) noexcept
        {
            stop_iterator = it;
        }

        constexpr std::unique_ptr<machine_memento<class_type>> check_point() const noexcept
        {
            return std::make_unique<machine_memento<class_type>>(*this);
        }

        constexpr void rollback (std::unique_ptr<machine_memento<class_type>> mem) noexcept
        {
            parent_class_type::reset(mem->b, mem->e);
        }

    public:
        [[nodiscard]] constexpr const_iterator get_start() const noexcept
        {
            return start_iterator;
        }

        [[nodiscard]] constexpr const_iterator get_stop() const noexcept
        {
            return stop_iterator;
        }

        virtual void process()
        {
            minmea_sentence_rmc frame {};
            if (minmea_parse_rmc(&frame, begin()+6, end()))
            {
                RMC_Callback::callback(frame);
            }
        }
    };
    template <size_t bs, class RMC_Callback>
    char machine<bs, RMC_Callback>::buffer[bs] = {};
}

