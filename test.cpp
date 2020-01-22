//
// Created by user on 13.01.2020.
//

#define BOOST_TEST_MODULE boost_test_module_
#include <boost/test/unit_test.hpp> // UTF ??
#include "machine.h"
#include <iostream>

struct test_machine : public serial::machine {
    using machine::fill_data;
    using machine::current_state;
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
    char external_buffer[6] = {'1','2','3','4','5','6'};

    m.fill_data(external_buffer, sizeof(external_buffer));
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
    char external_buffer[9] = {'1','2','3','4','5','6','$','G','+'};

    m.fill_data(external_buffer, sizeof(external_buffer));
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
    char external_buffer[10] = {'1','2','3','4','5','6','$','G','P','R'};

    m.fill_data(external_buffer, sizeof(external_buffer));

    while (m.parse()) {}

    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_rmc_state().get());
    BOOST_REQUIRE_EQUAL(m.size() , 3);
    BOOST_REQUIRE (m.get_start() == m.begin());
}

BOOST_AUTO_TEST_CASE( test_wrong_rmc_and_switch_$_state )
{
    using namespace serial;
    test_machine m;
    char const external_buffer [12] = {'1','2','3','4','5','6','$','G','R','M','C','_'}; // no 'R' in the middle

    m.fill_data(external_buffer, sizeof(external_buffer));

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
    char const external_buffer[13] = {'1','2','3','4','5','6','$','G','P','R','M','C','_'};

    m.fill_data(external_buffer, sizeof(external_buffer));

    while (m.parse()) {}

    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_crlf_state().get());
    BOOST_REQUIRE_EQUAL(m.size() , 1);
    BOOST_REQUIRE_EQUAL(*m.begin() , '_');
}

BOOST_AUTO_TEST_CASE( test_crlf_state_no_CR_LF_and_max_search_size )
{
    using namespace serial;
    test_machine m;
    char external_buffer[90] = {'$','G','P','R','M','C',};

    m.fill_data(external_buffer, sizeof(external_buffer));

    while (m.parse()) {}

    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_$_state().get());
    BOOST_REQUIRE_EQUAL(m.size() , 0);
}

BOOST_AUTO_TEST_CASE( test_crlf_state_CR_only )
{
    using namespace serial;
    test_machine m;
    char external_buffer[14] = {'1','2','3','4','5','6','$','G','P','R','M','C','_','\x0D'};

    m.fill_data(external_buffer, sizeof(external_buffer));

    while (m.parse()) {}

    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_crlf_state().get());
    BOOST_REQUIRE_EQUAL(m.size() , 1);
    BOOST_REQUIRE_EQUAL(*m.begin() , '\x0D');
}

BOOST_AUTO_TEST_CASE( test_crlf_state_CR_LF )
{
    using namespace serial;
    test_machine m;
    char external_buffer[15] = {'1','2','3','4','5','6','$','G','P','R','M','C','_','\x0D','\x0A'};

    m.fill_data(external_buffer, sizeof(external_buffer));

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
    char external_buffer[15] = {'1','2','3','4','5','6','$','G','P','R','M','C','_','\x0D','\x0A'};

    m.fill_data(external_buffer, sizeof(external_buffer));

    m.parse(); // $
    m.parse(); // RMC
    m.parse(); // CRLF
    auto pres = m.parse(); //CS -> $

    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_$_state().get());
    BOOST_REQUIRE (pres);
}

BOOST_AUTO_TEST_CASE( test_checksum_state_2)
{
    using namespace serial;
    test_machine m;
    char external_buffer[17] = {'1','2','3','4','5','6','$','G','P','R','M','C','*','_','_','\x0D','\x0A'};

    m.fill_data(external_buffer, sizeof(external_buffer));

    m.parse(); // $
    m.parse(); // RMC
    m.parse(); // CRLF
    auto pres = m.parse(); //CS -> $

    BOOST_REQUIRE_EQUAL(m.current_state, m.get_parse_checksum_state().get());
    BOOST_REQUIRE (pres);
}
