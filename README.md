# rmc_parser
NMEA RMC Message Parser

rmc_parser is a quite fast, stream-oriented, and free of heap allocations RMC messages parser.

Use it in contexts where, for example, fgets() call is not applicable (no input streams) or/and where there is an asynchronous delivery of message chunks into different sinks.

Under the hood rmc_parser is based on a storable, searchable and STL-compatible circular buffer of customizable length to support the responsiveness of your asynchronous system. 


