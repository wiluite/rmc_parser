#include <machine.h>
#include <random>
#include <iomanip>

struct message_callback
{
    static void callback(serial::minmea_sentence_rmc const & rmc)
    {
        std::cout << "Date: " << std::setfill('0') <<
        std::setw(2) << rmc.date.day << std::setw(2) << rmc.date.month << std::setw(2) << rmc.date.year << '\n';
        std::cout << "Time: " << std::setfill('0') <<
        std::setw(2) << rmc.time.hours << std::setw(2) << rmc.time.minutes << std::setw(2) << rmc.time.seconds <<
        '.' << rmc.time.microseconds << '\n';
    }
};

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis (10,50);
    serial::machine<300, message_callback> m;
    char const external_buffer[] = "$GPRMC,072633.327,A,5230.215,N,01324.658,E,847.2,283.3,140220,000.0,W*74\x0D\x0A"
                                   "$GPRMC,072634.327,A,5230.299,N,01324.297,E,383.9,000.0,140220,000.0,W*72\x0D\x0A"
                                   "BROKEN,072635.327,BROKEN.406,N,01324.297,E,383.9,000.0,140220,000.0,W*73\x0D\x0A"
                                   "$GPRMC,072636.327,A,5230.406,N,01324.297,E,082.2,025.1,140220,000.0,W*7F\x0D\x0A"
                                   "$GPRMC,BROKEN.327,A,5230.428,N,01324.307,E,082.2,000.0,140220,000.0,W*7C\x0D\x0A"
                                   "$GPRMC,072638.327,A,5230.428,N,01324.307,E,487.1,237.7,140220,000.0,W*70\x0D\x0A"
                                   "$GPRMC,072639.327,A,5230.331,N,01324.153,E,487.1,000.0,140220,000.0,W*7C\x0D\x0A"
                                   "$GPRMC,072640.327,A,5230.331,N,01324.153,E,000.0,000.0,140220,000.0,W*78\x0D\x0A";
    size_t count = 0;
    while (count < sizeof(external_buffer))
    {
        auto const rand_num = dis(gen);
        m.fill_data(external_buffer + count, (rand_num + count < sizeof(external_buffer) ? rand_num : sizeof(external_buffer)-count));
        count += rand_num;
        while (m.parse());
    };
    return 0;
}


