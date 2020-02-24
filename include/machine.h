#pragma once

#include <ring_iter.h>
#include <tokenizer.h>
#include <mach_mem.h>
#include <states.h>
#include <functional>

#ifdef _MSC_VER
#pragma warning(disable : 4355)
#endif

namespace serial
{
    using namespace funny_it;

    using state_destructor_type = std::function<void(void*)>;
    using state_ptr = std::unique_ptr<state, state_destructor_type>;

    template <size_t bs, class E = exception_checked_variant_type>
    using machine_implementation_type = ring_buffer_sequence<char, bs, E>;

    template <size_t bs, class RMC_Callback>
    class machine : private machine_implementation_type<bs>
    {
    private:
        static char buffer [bs];
        using parent_class_type = machine_implementation_type<bs>;
    public:
        using parent_class_type::align;
        using parent_class_type::size;
        using parent_class_type::distance;
        using parent_class_type::begin;
        using parent_class_type::end;
        using parent_class_type::fill_data;
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

        state_ptr const parse_$_state_;
        state_ptr const parse_rmc_state_;
        state_ptr const parse_crlf_state_;
        state_ptr const parse_checksum_state_;

    protected:
        state* current_state {nullptr};

        static_assert(sizeof(parse_$_state_type) <= sizeof(parse_$_state_type::storage));
        static_assert(sizeof(parse_rmc_state_type) <= sizeof(parse_rmc_state_type::storage));
        static_assert(sizeof(parse_crlf_state_type) <= sizeof(parse_crlf_state_type::storage));
        static_assert(sizeof(parse_checksum_state_type) <= sizeof(parse_checksum_state_type::storage));

    public:
        machine() : parent_class_type(buffer)
                , parse_$_state_(new(parse_$_state_type::storage)parse_$_state_type(*this), [](void * obj){static_cast<parse_$_state_type*>(obj)->~parse_$_state_type();})
                , parse_rmc_state_(new(parse_rmc_state_type::storage)parse_rmc_state_type(*this), [](void * obj){static_cast<parse_rmc_state_type*>(obj)->~parse_rmc_state_type();})
                , parse_crlf_state_(new(parse_crlf_state_type::storage)parse_crlf_state_type(*this), [](void * obj){static_cast<parse_crlf_state_type*>(obj)->~parse_crlf_state_type();})
                , parse_checksum_state_(new(parse_checksum_state_type::storage)parse_checksum_state_type(*this), [](void * obj){static_cast<parse_checksum_state_type*>(obj)->~parse_checksum_state_type();})
        {
            current_state = parse_$_state_.get();
        }
        virtual ~machine() = default;

        [[nodiscard]] constexpr bool parse() const noexcept
        {
            return current_state->parse();
        }

        [[nodiscard]] constexpr state_ptr const & get_parse_$_state() const noexcept
        {
            return parse_$_state_;
        }

        [[nodiscard]] constexpr state_ptr const & get_parse_rmc_state() const noexcept
        {
            return parse_rmc_state_;
        }

        [[nodiscard]] constexpr state_ptr const & get_parse_crlf_state() const noexcept
        {
            return parse_crlf_state_;
        }

        [[nodiscard]] constexpr state_ptr const & get_parse_checksum_state() const noexcept
        {
            return parse_checksum_state_;
        }

        constexpr void set_state (state_ptr const & state ) noexcept
        {
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

        [[nodiscard]] constexpr machine_memento<class_type> check_point() const noexcept
        {
            return machine_memento<class_type>(*this);
        }

        constexpr void rollback (machine_memento<class_type> const & mem) noexcept
        {
            parent_class_type::reset(mem.b, mem.e);
        }

        constexpr void unchecked_rollback (machine_memento<class_type> const & mem) noexcept
        {
            parent_class_type::unchecked_reset(mem.b, mem.e);
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

