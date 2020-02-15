# rmc_parser
NMEA RMC Message Parser

rmc_parser is a stream-oriented, ringbuffer-based, and free of heap allocations RMC messages parser.
Use it in contexts where fgets() is not applicable (no input streams) or where there is asynchronous delivery of message chunks.
