.. _bt_mesh_time_tai_readme:

International atomic time (TAI)
###############################

Contrary to UTC, TAI operates on an absolutely linear timeline, based on the atomic second.

As earth's rotational speed changes by a couple of milliseconds per century, the number of seconds in a solar day has changed since the second was defined in the 19th century.
The length of a solar day is now slightly longer than the original 86400 seconds, and to compensate, UTC adds a "leap second" every 800 days or so.
Just like leap days, leap seconds are extra seconds added at a scheduled time to make up for the accumulated offset since the last leap second.
The leap seconds allow UTC to follow the solar day (which otherwise would be skewed by a few milliseconds every year), but cause some minutes to be more than 60 seconds long.
Leap seconds are not regular events, but are scheduled by a governing board about 6 months ahead of time.
This makes it impossible to predict future leap seconds, which makes it impossible to determine the number of UTC seconds between two future dates.
At the time of writing, UTC is 37 seconds behind TAI (since July 2019).

Instead of moving the timestamp with leap seconds, TAI never performs any jumps, but keeps running at a rate of one second per second forever.
This allows applications to calculate the number of seconds between any two TAI timestamps, while this is not possible with UTC due to its irregular leap seconds.

To convert to UTC, TAI based applications keep track of the UTC leap seconds separately, as well as the time zone and time zone adjustments.

The Bluetooth® Mesh Time models share time as a composite state of TAI seconds, 256 subseconds, UTC offset, time zone steps and uncertainty.
See :cpp:type:`bt_mesh_time_status` for details.
