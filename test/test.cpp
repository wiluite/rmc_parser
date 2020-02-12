#define BOOST_TEST_MODULE boost_test_module_
#include <boost/test/unit_test.hpp> // UTF ??
#include <machine.h>
#include <parser.h>

struct rmc_callback1 // callback for test_machine class
{
    static void callback(serial::minmea_sentence_rmc rmc)
    {
        BOOST_CHECK ((rmc.latitude.value == -375165) || (rmc.latitude.value == 491645));
        BOOST_CHECK (rmc.latitude.scale == 100);
        BOOST_CHECK ((rmc.longitude.value == 1450736) || (rmc.longitude.value == -1231112));
        BOOST_CHECK (rmc.longitude.scale == 100);
        BOOST_CHECK ((rmc.speed.value == 0) || (rmc.speed.value == 5));
        BOOST_CHECK (rmc.speed.scale == 10);
        BOOST_CHECK ((rmc.time.hours == 8) || (rmc.time.hours == 22));
        BOOST_CHECK ((rmc.time.minutes == 18) || (rmc.time.minutes == 54));
        BOOST_CHECK ((rmc.time.seconds == 36) || (rmc.time.seconds == 46));
        BOOST_CHECK (rmc.time.microseconds == 0);
        BOOST_CHECK_EQUAL (rmc.date.year, 19);
        BOOST_CHECK ((rmc.date.month == 9) || (rmc.date.month == 11));
        BOOST_CHECK ((rmc.date.day == 19) || (rmc.date.day == 13));
    }
};
struct test_machine : public serial::machine<200, rmc_callback1> {
    using parent_class_type = serial::machine<200, rmc_callback1>;
    using machine::fill_data;
    using machine::current_state;
    int proc_call = 0;
    void process() override // with parent's method called
    {
        ++proc_call;
        parent_class_type::process();
    }
};


struct rmc_callback2 // callback for rotated_parse_test_machine class
{
    static void callback(serial::minmea_sentence_rmc rmc)
    {
        // add checks yourself and ensure all values are right.
    }
};
template <size_t bs, class callback=rmc_callback2>
struct rotated_parse_test_machine : public serial::machine<bs, callback> {
    using parent_class_type = serial::machine<bs, callback>;
    using parent_class_type::fill_data;
    using parent_class_type::current_state;
    using parent_class_type::begin;
    using parent_class_type::end;
    int proc_call = 0;
    void process() override // standalone processing
    {
        ++proc_call;
        serial::minmea_sentence_rmc frame {};
        if (minmea_parse_rmc(&frame, begin()+6, end()))
        {
            callback::callback(frame);
        }
    }
};

BOOST_AUTO_TEST_CASE( test_empty_machine_data )
{
    using namespace serial;
    test_machine m;
    bool parse_result {};
    BOOST_REQUIRE(m.size() == 0);
    BOOST_REQUIRE_NO_THROW(parse_result = m.parse());
    BOOST_REQUIRE_EQUAL(parse_result, false);
    BOOST_REQUIRE(m.current_state == m.get_parse_$_state().get());
}

BOOST_AUTO_TEST_CASE( test_$_and_same_state )
{
    using namespace serial;
    test_machine m;
    char external_buffer[] = {"123456"};

    m.fill_data(external_buffer, sizeof(external_buffer)-1);
    BOOST_REQUIRE(m.size() == 6);
    bool parse_result {};
    BOOST_REQUIRE_NO_THROW(parse_result = m.parse());
    BOOST_REQUIRE_EQUAL(parse_result, false);
    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_$_state().get());
    BOOST_REQUIRE_EQUAL(m.size() , 0);
}

BOOST_AUTO_TEST_CASE( test_$_and_switch_state )
{
    using namespace serial;
    test_machine m;
    char external_buffer[] = {"123456$G+"};

    m.fill_data(external_buffer, sizeof(external_buffer)-1);
    bool parse_result {};
    parse_result = m.parse();
    BOOST_REQUIRE_EQUAL(parse_result, true);
    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_rmc_state().get());
    BOOST_REQUIRE_EQUAL (*m.get_start() ,(int)'G');
    BOOST_REQUIRE_EQUAL (*m.begin() ,(int)'G');
    BOOST_REQUIRE_EQUAL(m.size() , 2);
}

BOOST_AUTO_TEST_CASE( test_insufficient_rmc_and_same_state )
{
    using namespace serial;
    test_machine m;
    char external_buffer[] = {"123456$GPR"};

    m.fill_data(external_buffer, sizeof(external_buffer)-1);

    while (m.parse()) {}

    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_rmc_state().get());
    BOOST_REQUIRE_EQUAL(m.size() , 3);
    BOOST_REQUIRE (m.get_start() == m.begin());
}

BOOST_AUTO_TEST_CASE( test_wrong_rmc_and_switch_$_state )
{
    using namespace serial;
    test_machine m;
    char const external_buffer [] = {"123456$GRMC_"}; // no 'P' in the middle

    m.fill_data(external_buffer, sizeof(external_buffer)-1);

    auto pres {false};
    while ( (pres = m.parse())) {}

    BOOST_REQUIRE(!pres);
    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_$_state().get());
    BOOST_REQUIRE_EQUAL(m.size() , 0);
    BOOST_REQUIRE (m.get_start() != m.begin());

}

BOOST_AUTO_TEST_CASE( test_rmc_and_switch_state )
{
    using namespace serial;
    test_machine m;
    char const external_buffer[] = {"123456$GPRMC_"};

    m.fill_data(external_buffer, sizeof(external_buffer)-1);

    while (m.parse()) {}

    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_crlf_state().get());
    BOOST_REQUIRE_EQUAL(m.size() , 1);
    BOOST_REQUIRE_EQUAL(*m.begin() , '_');
}

BOOST_AUTO_TEST_CASE( test_crlf_state_no_CR_LF_and_max_search_size )
{
    using namespace serial;
    test_machine m;
    char external_buffer[90] = {"$GPRMC"};

    m.fill_data(external_buffer, sizeof(external_buffer));

    while (m.parse()) {}

    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_$_state().get());
    BOOST_REQUIRE_EQUAL(m.size() , 0);
}

BOOST_AUTO_TEST_CASE( test_crlf_state_CR_only )
{
    using namespace serial;
    test_machine m;
    char external_buffer[] = {"123456$GPRMC_\x0D"};

    m.fill_data(external_buffer, sizeof(external_buffer)-1);

    while (m.parse()) {}

    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_crlf_state().get());
    BOOST_REQUIRE_EQUAL(m.size() , 1);
    BOOST_REQUIRE_EQUAL(*m.begin() , '\x0D');
}

BOOST_AUTO_TEST_CASE( test_crlf_state_CR_LF )
{
    using namespace serial;
    test_machine m;
    char external_buffer[] = {"123456$GPRMC_\x0D\x0A"};

    m.fill_data(external_buffer, sizeof(external_buffer)-1);

    m.parse(); // $
    m.parse(); // RMC
    auto pres = m.parse(); // CRLF


    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_checksum_state().get());
    BOOST_REQUIRE (pres);
}

BOOST_AUTO_TEST_CASE( test_checksum_state_1)
{
    using namespace serial;
    test_machine m;
    char external_buffer[] = {"123456$GPRMC_\x0D\x0A"};

    m.fill_data(external_buffer, sizeof(external_buffer)-1);

    m.parse(); // $
    m.parse(); // RMC
    m.parse(); // CRLF
    auto pres = m.parse(); //CS -> $

    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_$_state().get());
    BOOST_REQUIRE (pres);
    BOOST_REQUIRE_EQUAL (m.size(), 0);
}

BOOST_AUTO_TEST_CASE( test_checksum_state_2)
{
    using namespace serial;
    test_machine m;
    char external_buffer[] = {"123456$GPRMC*__\x0D\x0A"};

    m.fill_data(external_buffer, sizeof(external_buffer)-1);

    auto const save_proc_call = m.proc_call;

    m.parse(); // $
    m.parse(); // RMC
    m.parse(); // CRLF
    auto pres = m.parse(); //CS -> $

    BOOST_REQUIRE_EQUAL(m.proc_call, save_proc_call);
    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_$_state().get());
    BOOST_REQUIRE (pres);
    BOOST_REQUIRE_EQUAL (m.size(), 0);
}

BOOST_AUTO_TEST_CASE( test_successful_checksum)
{
    using namespace serial;
    test_machine m;
    char external_buffer[] = {"$GPRMC,081836.000,A,3751.65,S,14507.36,E,000.0,360.0,130919,011.3,E*75\x0D\x0A"};
    m.fill_data(external_buffer, sizeof(external_buffer));

    auto const save_proc_call = m.proc_call;
    m.parse(); // $
    m.parse(); // RMC
    m.parse(); // CRLF
    auto pres = m.parse(); // CS -> $

    BOOST_REQUIRE_EQUAL(m.proc_call, save_proc_call + 1);
    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_$_state().get());
    BOOST_REQUIRE (pres);
}

BOOST_AUTO_TEST_CASE( test_2_messages)
{
    using namespace serial;
    test_machine m;
    auto const save_proc_call = m.proc_call;

    char external_buffer[] = {"$GPRMC,081836.000,A,3751.65,S,14507.36,E,000.0,360.0,130919,011.3,E*75\x0D\x0A$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191119,020.3,E*6D\x0D\x0A"};
    m.fill_data(external_buffer, sizeof(external_buffer)/3);
    while(m.parse()) {}
    m.fill_data(external_buffer + sizeof(external_buffer)/3, (sizeof(external_buffer) - sizeof(external_buffer)/3));
    while(m.parse()) {}

    BOOST_REQUIRE_EQUAL(m.proc_call, save_proc_call + 2);
    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_$_state().get());
}

template <int T>
constexpr void rotated_parse (std::integral_constant<int, T>  sz)
{
    rotated_parse_test_machine<sz> m;
    char external_buffer1[] = {"$GPRMC,081836.000,A,3751.65,S,14507.36,E,000.0,360.0,130919,011.3,E*75\x0D\x0A"};
    char external_buffer2[] = {"$GPRMC,,V,,,,,,,080907,9.6,E,N*31\x0D\x0A"};

    auto const save_proc_call = m.proc_call;

    m.fill_data(external_buffer1, sizeof(external_buffer1));
    while(m.parse()) {}

    m.fill_data(external_buffer2, sizeof(external_buffer2));
    while(m.parse()) {}

    BOOST_REQUIRE_EQUAL(m.proc_call, save_proc_call + 2);
    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_$_state().get());
}

BOOST_AUTO_TEST_CASE( test_rotated_parse)
{
    using namespace serial;

    rotated_parse(std::integral_constant<int, 74>());
    rotated_parse(std::integral_constant<int, 81>());
    rotated_parse(std::integral_constant<int, 85>());
    rotated_parse(std::integral_constant<int, 92>());
    rotated_parse(std::integral_constant<int, 96>());
    rotated_parse(std::integral_constant<int, 100>());
}


std::size_t memory = 0;
std::size_t alloc = 0;
void* operator new(std::size_t s) noexcept(false)
{
    memory += s;
    ++alloc;
    return malloc(s);
}
void operator delete(void* p) throw()
{
    --alloc;
    free(p);
}
std::tuple<std::size_t, std::size_t> memory_use()
{
    return {memory, alloc};
}

BOOST_AUTO_TEST_CASE (test_memory_allocations)
{
    using namespace serial;

    auto [mem1, alloc1] = memory_use();
    test_machine m;
    char external_buffer[] = {"$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130919,011.3,E*6B\x0D\x0A"};
    m.fill_data(external_buffer, sizeof(external_buffer));
    auto const save_proc_call = m.proc_call;
    while (m.parse()) ;
    auto [mem2, alloc2] = memory_use();
    BOOST_CHECK_EQUAL (mem1, mem2);
    BOOST_CHECK_EQUAL (alloc1, alloc2);
    BOOST_CHECK_EQUAL(m.proc_call, save_proc_call + 1);
}
