.. _icalendar_parser_readme:

iCalendar parser
################

.. contents::
   :local:
   :depth: 2

The iCalendar parser library can be used to parse a data stream in iCalendar format, which is a data format for representing and exchanging calendaring and scheduling information.
The library parses the calendaring information and returns parsed calendar events.

The library first detects the beginning of the calendar object by locating the delimiter ``BEGIN:VCALENDAR``.
It then parses the following calendar content fragment by fragment.
For each calendar component that is parsed, the library sends a parsed event (:c:struct:`ical_parser_evt`) to the application.

Supported features
******************

The library supports iCalendar version 2.0 as defined in RFC5545.

The iCalendar parser library supports parsing the following calendar components:

* VEVENT

It supports parsing the following calendar properties:

* DESCRIPTION
* DTEND
* DTSTART
* LOCATION
* SUMMARY

API documentation
*****************

| Header file: :file:`include/net/icalendar_parser.h`
| Source files: :file:`subsys/net/lib/icalendar_parser/src/`

.. doxygengroup:: icalendar_parser
   :project: nrf
   :members:
