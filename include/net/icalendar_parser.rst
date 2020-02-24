.. _icalendar_parser_readme:

iCalendar parser
###############

`iCalendar`_ is data format for representing and exchanging calendaring and scheduling information. The iCalendar parser library can be used to parse a shared iCalendar at specified URL (HTTP or HTTPS) and returns paresd calendar events. It supports IPv4 and IPv6 protocols.

The library detect the begin of calendar delimiter(BEGIN:VCALENDAR) and sends an event (:cpp:member:`ICAL_PARSER_EVT_CALENDAR`) to the application. Then the library parse calendar content fragment by fragment and sends parsed events (:cpp:member:`ICAL_PARSER_EVT_COMPONENT`) to the application when calendar components are parsed.
When the end of calendar delimiter(END:VCALENDAR) is parsed, the library sends an event (:cpp:member:`ICAL_PARSER_EVT_COMPLETE`) to the application.

If some error occur during calendar downloading and parsing, the library sends an event (:cpp:member:`ICAL_PARSER_EVT_ERROR`) and disconnet from the server.

Protocols
*********

The library supports HTTP and HTTPS (TLS 1.2) over IPv4 and IPv6.

Limitations
***********

Currently the iCalendar parser library supports parsing of below calendar components/properties:

Calendar components:
    * VEVENT

Calendar properties:
    * DTSTART
    * DTEND
    * SUMMARY
    * LOCATION
    * DESCIPTION

API documentation
*****************

| Header file: :file:`include/net/icalendar_parser.h`
| Source files: :file:`subsys/net/lib/icalendar_parser/src/`

.. doxygengroup:: icalendar_parser
   :project: nrf
   :members:

References
**********

.. _iCalendar:
   https://tools.ietf.org/html/rfc5545