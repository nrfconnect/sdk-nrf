.. _SLM_AT_GPS:

GPS AT commands
***************

.. contents::
   :local:
   :depth: 2

The following commands list contains GPS related AT commands.

Run GPS #XGPS
=============

The ``#XGPS`` command controls the GPS.

Set command
-----------

The set command allows you to start and stop the GPS.

Syntax
~~~~~~

::

   #XGPS=<op>[,<mask>]

The ``<op>`` parameter accepts the following integer values:

* ``0`` - Start GPS
* ``1`` - Stop GPS

The ``<mask>`` parameter represents the NMEA data mask.
It accepts the following integer values:

* Bit 0 - Global Positioning System fix data
* Bit 1 - Geographic position latitude/longitude and time
* Bit 2 - DOP and active satellites
* Bit 3 - Satellites in view
* Bit 4 - Recommended minimum specific GPS/transit data

They are all set if the NMEA data mask value is ignored.

Response syntax
~~~~~~~~~~~~~~~

::

   #XGPS: <status>[,<mask>]

The ``<status>`` value represents the GPS running status.
It can have the following values:

* ``0`` - Stopped.
* ``1`` - Running.
* *Negative Value* - Error code.
  It indicates the reason for the failure.

When the ``<status>`` value is 1, the ``<mask>`` value syntax appears as follows:

* Bit 0 - Global Positioning System Fix Data
* Bit 1 - Geographic position latitude/longitude and time
* Bit 2 - DOP and active satellites
* Bit 3 - Satellites in view
* Bit 4 - Recommended minimum specific GPS/transit data

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XGPS: <status>
   #XGPSS: <satellite>
   #XGPSP: <position>
   <nmea>

* The ``<status>`` value represents the GPS running status.
  It can have the following values:

  * ``0`` - Stopped.
  * ``1`` - Running.
  * *Negative Value* - Error code.
    It indicates the reason for the failure.

* The ``<satellite>`` value represents the satellite statistic info.
* The ``<position>`` value represents longitude, latitude, and UTC DateTime.
* The ``<nmea>`` value represents the NMEA raw data, notified after the first satellite fix.

Example
~~~~~~~

::

   #XGPS: 1,3
   OK
   #XGPSS: "GPS suspended"
   #XGPSS: "SUPL injection done"
   #XGPSS: "GPS resumed"
   #XGPSS: "track 3 use 3 unhealthy 0"
   #XGPSS: "track 4 use 4 unhealthy 0"
   #XGPSS: "track 5 use 5 unhealthy 0"
   #XGPSS: "track 4 use 4 unhealthy 0"
   #XGPSS: "track 5 use 5 unhealthy 0"
   #XGPSS: "track 6 use 6 unhealthy 0"
   #XGPSS: "track 7 use 7 unhealthy 0"
   #XGPSS: "track 6 use 6 unhealthy 0"
   #XGPSP: "long 139.721966 lat 35.534159"
   #XGPSP: "2020-04-30 00:11:55"
   #XGPSP: "TTFF 57s"
   $GPGGA,001155.87,3532.04954,N,13943.31794,E,1,06,17.40,109.53,M,0,,*19
   $GPGLL,3532.04954,N,13943.31794,E,001155.87,A,A*69
   #XGPSP: "long 139.721969 lat 35.534148"
   #XGPSP: "2020-04-30 00:11:56"

Read command
------------

The read command checks if the GPS is running.

Syntax
~~~~~~

::

   #XGPS?

Response syntax
~~~~~~~~~~~~~~~

::

   #XGPS: <status>[,<mask>]

The ``<status>`` parameter represents the GPS running status.
It can accept the following values:

* ``0`` - Stopped
* ``1`` - Running

When the ``<status>`` value is 1, the ``<mask>`` value syntax appears as follows:

* Bit 0 - Global Positioning System fix data
* Bit 1 - Geographic position latitude/longitude and time
* Bit 2 - DOP and active satellites
* Bit 3 - Satellites in view
* Bit 4 - Recommended minimum specific GPS/transit data

Example
~~~~~~~

::

   AT#XGPS?
   #XGPS: 1,2
   OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XGPS=?

Example
~~~~~~~

::

   #XGPS: (0,1),<mask>
   NMEA data mask:
   Bit 0 - Global Positioning System fix data
   Bit 1 - Geographic position latitude/longitude and time
   Bit 2 - DOP and active satellites
   Bit 3 - Satellites in view
   Bit 4 - Recommended minimum specific GPS/transit data
   OK
