#pragma once

#include <cstdarg>
#include <cmath>

// This code is borrowed from minmea parser by Kosma Moczek and has been slightly modified.
// It is armed with ring buffer iterator used throughout the parser environment.

namespace serial
{
    struct minmea_float {
        int_least32_t value;
        int_least32_t scale;
    };

    struct minmea_date {
        int day;
        int month;
        int year;
    };

    struct minmea_time {
        int hours;
        int minutes;
        int seconds;
        int microseconds;
    };

    struct minmea_sentence_rmc {
        struct minmea_time time;
        bool valid;
        struct minmea_float latitude;
        struct minmea_float longitude;
        struct minmea_float speed;
        struct minmea_float course;
        struct minmea_date date;
        struct minmea_float variation;
    };

    static inline bool minmea_isfield(char c) {
        return isprint((unsigned char) c) && c != ',' && c != '*';
    }

    template <typename RING_IT>
    bool minmea_scan(RING_IT it, RING_IT it2, const char *format, ...)
    {
        bool result = false;
        bool optional = false;
        va_list ap;
        va_start(ap, format);

        auto field = it;
#define next_field() \
    do { \
        /* Progress to the next field. */ \
        while (minmea_isfield(*it)) \
            ++it; \
        /* Make sure there is a field there. */ \
        if (*it == ',') { \
            ++it; \
            field = it; \
        } else { \
            field = it2; \
            assert (!field); \
        } \
    } while (0)

        while (*format) {
            char type = *format++;

            if (type == ';') {
                // All further fields are optional.
                optional = true;
                continue;
            }

            if (!field && !optional)
            {
                // Field requested but we ran out if input. Bail out.
                goto parse_error;
            }

            switch (type) {
                case 'c': { // Single character field (char).
                    char value = '\0';

                    if (field && minmea_isfield(*field))
                        value = *field;

                    *va_arg(ap, char *) = value;
                } break;

                case 'd': { // Single character direction field (int).
                    int value = 0;

                    if (field && minmea_isfield(*field)) {
                        switch (*field) {
                            case 'N':
                            case 'E':
                                value = 1;
                                break;
                            case 'S':
                            case 'W':
                                value = -1;
                                break;
                            default:
                                goto parse_error;
                        }
                    }

                    *va_arg(ap, int *) = value;
                } break;

                case 'f': { // Fractional value with scale (struct minmea_float).
                    int sign = 0;
                    int_least32_t value = -1;
                    int_least32_t scale = 0;

                    if (field) {
                        while (minmea_isfield(*field)) {
                            if (*field == '+' && !sign && value == -1) {
                                sign = 1;
                            } else if (*field == '-' && !sign && value == -1) {
                                sign = -1;
                            } else if (isdigit((unsigned char) *field)) {
                                int digit = *field - '0';
                                if (value == -1)
                                    value = 0;
                                if (value > (INT_LEAST32_MAX-digit) / 10) {
                                    /* we ran out of bits, what do we do? */
                                    if (scale) {
                                        /* truncate extra precision */
                                        break;
                                    } else {
                                        /* integer overflow. bail out. */
                                        goto parse_error;
                                    }
                                }
                                value = (10 * value) + digit;
                                if (scale)
                                    scale *= 10;
                            } else if (*field == '.' && scale == 0) {
                                scale = 1;
                            } else if (*field == ' ') {
                                /* Allow spaces at the start of the field. Not NMEA
                                 * conformant, but some modules do this. */
                                if (sign != 0 || value != -1 || scale != 0)
                                    goto parse_error;
                            } else {
                                goto parse_error;
                            }
                            ++field;
                        }
                    }

                    if ((sign || scale) && value == -1)
                        goto parse_error;

                    if (value == -1) {
                        /* No digits were scanned. */
                        value = 0;
                        scale = 0;
                    } else if (scale == 0) {
                        /* No decimal point. */
                        scale = 1;
                    }
                    if (sign)
                        value *= sign;

                    *va_arg(ap, struct minmea_float *) = (struct minmea_float) {value, scale};
                } break;

                case 'D': { // Date (int, int, int), -1 if empty.
                    struct minmea_date *date = va_arg(ap, struct minmea_date *);

                    int d = -1, m = -1, y = -1;

                    if (field && minmea_isfield(*field)) {
                        // Always six digits.
                        for (int f=0; f<6; f++)
                            if (!isdigit((unsigned char) field[f]))
                                goto parse_error;

                        char dArr[] = {field[0], field[1], '\0'};
                        char mArr[] = {field[2], field[3], '\0'};
                        char yArr[] = {field[4], field[5], '\0'};
                        d = strtol(dArr, nullptr, 10);
                        m = strtol(mArr, nullptr, 10);
                        y = strtol(yArr, nullptr, 10);
                    }

                    date->day = d;
                    date->month = m;
                    date->year = y;
                } break;

                case 'T': { // Time (int, int, int, int), -1 if empty.
                    struct minmea_time *time_ = va_arg(ap, struct minmea_time *);

                    int h = -1, i = -1, s = -1, u = -1;

                    if (field && minmea_isfield(*field)) {
                        // Minimum required: integer time.
                        for (int f=0; f<6; f++)
                            if (!isdigit((unsigned char) field[f]))
                                goto parse_error;

                        char hArr[] = {field[0], field[1], '\0'};
                        char iArr[] = {field[2], field[3], '\0'};
                        char sArr[] = {field[4], field[5], '\0'};
                        h = strtol(hArr, nullptr, 10);
                        i = strtol(iArr, nullptr, 10);
                        s = strtol(sArr, nullptr, 10);
                        field += 6;

                        // Extra: fractional time. Saved as microseconds.
                        if (*field++ == '.') {
                            uint32_t value = 0;
                            uint32_t scale = 1000000LU;
                            while (isdigit((unsigned char) *field) && scale > 1) {
                                value = (value * 10) + (*field++ - '0');
                                scale /= 10;
                            }
                            u = value * scale;
                        } else {
                            u = 0;
                        }
                    }

                    time_->hours = h;
                    time_->minutes = i;
                    time_->seconds = s;
                    time_->microseconds = u;
                } break;

                default: { // Unknown.
                    goto parse_error;
                }
            }

            next_field();
        }

        result = true;

        parse_error:
        va_end(ap);
        return result;
    }

    template <typename RING_IT>
    bool minmea_parse_rmc(struct minmea_sentence_rmc *frame, RING_IT it, RING_IT end)
    {
        char validity;
        int latitude_direction;
        int longitude_direction;
        int variation_direction;
        if (!minmea_scan(it, end, "TcfdfdffDfd",
                         &frame->time,
                         &validity,
                         &frame->latitude, &latitude_direction,
                         &frame->longitude, &longitude_direction,
                         &frame->speed,
                         &frame->course,
                         &frame->date,
                         &frame->variation, &variation_direction))
            return false;

        frame->valid = (validity == 'A');
        frame->latitude.value *= latitude_direction;
        frame->longitude.value *= longitude_direction;
        frame->variation.value *= variation_direction;

        return true;
    }
}
