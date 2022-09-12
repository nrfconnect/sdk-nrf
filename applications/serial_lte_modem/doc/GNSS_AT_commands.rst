.. _SLM_AT_GNSS:

GNSS AT commands
****************

.. contents::
   :local:
   :depth: 2

The following commands list contains GNSS-related AT commands.

Run GNSS
========

The ``#XGPS`` command controls the GNSS.

Set command
-----------

The set command allows you to start and stop the GNSS.

Syntax
~~~~~~

::

   #XGPS=<op>[,<interval>[,<timeout>]]

The ``<op>`` parameter accepts the following integer values:

* ``0`` - Stop GNSS
* ``1`` - Start GNSS

The ``<interval>`` parameter represents the GNSS fix interval in seconds.
It must be set when starting the GNSS.
It accepts the following integer values:

* ``0`` - Single-fix navigation mode.
* ``1`` - Continuous navigation mode.
  The fix interval is set to 1 second
* Ranging from ``10`` to ``1800`` - Periodic navigation mode.
  The fix interval is set to the specified value.

In periodic navigation mode, the ``<timeout>`` parameter controls the maximum time in seconds that the GNSS receiver is allowed to run while trying to produce a valid PVT estimate.
In continuous navigation mode, this parameter does not have any effect.
It accepts the following integer values:

* ``0`` - The GNSS receiver runs indefinitely until a valid PVT estimate is produced.
* Any positive integer lower than the ``<interval>`` value - The GNSS receiver is turned off after the specified time is up, even if a valid PVT estimate was not produced.

When not specified, it defaults to a timeout value of 60 seconds.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XGPS: <latitude>,<longitude>,<altitude>,<accuracy>,<speed>,<heading>,<datetime>

* The ``<latitude>`` value represents the latitude in degrees.
* The ``<longitude>`` value represents the longitude in degrees.
* The ``<altitude>`` value represents the altitude above the WGS-84 ellipsoid in meters.
* The ``<accuracy>`` value represents the accuracy (2D 1-sigma) in meters.
* The ``<speed>`` value represents the horizontal speed in meters.
* The ``<heading>`` value represents the heading of the movement of the user in degrees.
* The ``<datetime>`` value represents the UTC date-time.

::

   #XGPS: <NMEA message>

The ``<NMEA message>`` is the ``$GPGGA`` (Global Positioning System Fix Data) NMEA sentence.

::

   #XGPS: <gnss_service>,<gnss_status>

Refer to the READ command.

Example
~~~~~~~

::

  AT%XSYSTEMMODE=0,0,1,0

  OK
  AT%XCOEX0=1,1,1565,1586

  OK
  AT+CFUN=31

  OK
  AT#XGPS=1,1

  #XGPS: 1,1

  OK

  #XGPS: 35.457576,139.625090,121.473785,22.199919,0.442868,0.000000,"2021-06-02 06:25:48"

  #XGPS: 35.457550,139.625115,124.293533,15.679427,0.263094,0.000000,"2021-06-02 06:25:49"

  #XGPS: 35.457517,139.625094,120.865372,12.768595,0.166673,0.000000,"2021-06-02 06:25:50"

Read command
------------

The read command allows you to check GNSS support and service status.

Syntax
~~~~~~

::

   #XGPS?

Response syntax
~~~~~~~~~~~~~~~

::

   #XGPS: <gnss_service>,<gnss_status>

* The ``<gnss_service>`` value is an integer.
  When it returns the value of ``1``, it means that GNSS is supported in ``%XSYSTEMMODE`` and activated in ``+CFUN``.

* The ``<gnss_status>`` value is an integer.

* ``0`` - GNSS is stopped.
* ``1`` - GNSS is started.
* ``2`` - GNSS wakes up in periodic mode.
* ``3`` - GNSS enters sleep because of timeout.
* ``4`` - GNSS enters sleep because a fix is achieved.

Example
~~~~~~~

::

  AT#XGPS?

  #XGPS: 1,1

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

  AT#XGPS=?

  #XGPS: (0,1),<interval>,<timeout>

  OK


Connect to nRF Cloud
====================

The ``#XNRFCLOUD`` command controls the connection to the nRF Cloud service.

.. note::
   To use ``#XNRFCLOUD``, you must first provision the device to nRF Cloud, using the UUID from the modem firmware as device ID.

Set command
-----------

The set command allows you to connect and disconnect the nRF Cloud service.

Syntax
~~~~~~

::

   #XNRFCLOUD=<op>[,<signify>]

The ``<op>`` parameter accepts the following integer values:

* ``0`` - Disconnect from the nRF Cloud service.
* ``1`` - Connect to the nRF Cloud service.
* ``2`` - Send a message in the JSON format to the nRF Cloud service.

When ``<op>`` is ``2``, SLM enters ``slm_data_mode``.

The ``<signify>`` parameter is used only when the ``<op>`` value is ``1``
It accepts the following integer values:

* ``0`` - It does not signify the location info to nRF Cloud.
* ``1`` - It does signify the location info to nRF Cloud.

When the ``<signify>`` parameter is not specified, it does not signify the location info to nRF Cloud.

.. note::
   The application signifies the location info to nRF Cloud in a best-effort way.
   The minimal report interval is 5 seconds.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XNRFCLOUD: <ready>,<signify>

* The ``<ready>`` value indicates whether the nRF Cloud connection is ready or not.
* The ``<signify>`` value indicates whether the location info will be signified to nRF Cloud or not.

::

   #XNRFCLOUD: <message>

* The ``<message>`` value indicates the nRF Cloud data received when A-GPS, P-GPS, and Cell_Pos are not active.

Example
~~~~~~~

::

  AT#XNRFCLOUD=1

  OK
  #XNRFCLOUD: 1,0

  AT#XNRFCLOUD=2
  {"msg":"Hello, nRF Cloud"}
  OK

  #XNRFCLOUD: {"msg":"Hello"}

  AT#XNRFCLOUD=0

  AT#XNRFCLOUD: 0,0

  OK
  AT#XNRFCLOUD=1,1

  OK
  #XNRFCLOUD: 1,1
  AT#XNRFCLOUD=0

  AT#XNRFCLOUD: 0,1

  OK

Read command
------------

The read command checks if nRF Cloud is connected or not.

Syntax
~~~~~~

::

   #XNRFCLOUD?

Response syntax
~~~~~~~~~~~~~~~

::

   #XNRFCLOUD: <ready>,<signify>,<sec_tag>,<device_id>

* The ``<ready>`` value indicates whether the nRF Cloud connection is ready or not.
* The ``<signify>`` value indicates whether the location info will be signified to nRF Cloud or not.
* The ``<sec_tag>`` value indicates the ``sec_tag`` used for accessing nRF Cloud.
* The ``<device_id>`` value indicates the device ID used for accessing nRF Cloud.

Example
~~~~~~~

::

  AT#XNRFCLOUD?

  #XNRFCLOUD: 1,0,16842753,"nrf-352656106443792"

  OK

::

  AT#XNRFCLOUD?

  #XNRFCLOUD: 1,0,8888,"50503041-3633-4261-803d-1e2b8f70111a"

  OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XNRFCLOUD=?

Example
~~~~~~~

::

  AT#XXNRFCLOUD=?

  #XNRFCLOUD: (0,1,2),<signify>

  OK

Run GNSS with nRF Cloud A-GPS
=============================

The ``#XAGPS`` command runs the GNSS together with the nRF Cloud A-GPS service.

.. note::
   To use ``#XAGPS``, the following preconditions apply:

   * You must define :ref:`CONFIG_SLM_AGPS <CONFIG_SLM_AGPS>`.
   * You must have access to nRF Cloud through the LTE network for receiving A-GPS data.

Set command
-----------

The set command allows you to start and stop the GNSS together with the nRF Cloud A-GPS service.

Syntax
~~~~~~

::

   #XAGPS=<op>[,<interval>[,<timeout>]]

The ``<op>`` parameter accepts the following integer values:

* ``0`` - Stop GNSS with A-GPS
* ``1`` - Start GNSS with A-GPS

The ``<interval>`` parameter represents the GNSS fix interval in seconds.
It must be set when starting the GNSS.
It accepts the following integer values:

* ``0`` - Single-fix navigation mode.
* ``1`` - Continuous navigation mode.
  The fix interval is set to 1 second
* Ranging from ``10`` to ``1800`` - Periodic navigation mode.
  The fix interval is set to the specified value.

In periodic navigation mode, the ``<timeout>`` parameter controls the maximum time in seconds that the GNSS receiver is allowed to run while trying to produce a valid PVT estimate.
In continuous navigation mode, this parameter does not have any effect.
It accepts the following integer values:

* ``0`` - The GNSS receiver runs indefinitely until a valid PVT estimate is produced.
* Any positive integer lower than the ``<interval>`` value - the GNSS receiver is turned off after the specified time is up, even if a valid PVT estimate was not produced.

When not specified, it defaults to a timeout value of 60 seconds.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XGPS: <latitude>,<longitude>,<altitude>,<accuracy>,<speed>,<heading>,<datetime>

* The ``<latitude>`` value represents the latitude in degrees.
* The ``<longitude>`` value represents the longitude in degrees.
* The ``<altitude>`` value represents the altitude above the WGS-84 ellipsoid in meters.
* The ``<accuracy>`` value represents the accuracy (2D 1-sigma) in meters.
* The ``<speed>`` value represents the horizontal speed in meters.
* The ``<heading>`` value represents the heading of the movement of the user in degrees.
* The ``<datetime>`` value represents the UTC date-time.

::

   #XGPS: <NMEA message>

The ``<NMEA message>`` is the ``$GPGGA`` (Global Positioning System Fix Data) NMEA sentence.

::

   #XAGPS: <gnss_service>,<agps_status>

Refer to the READ command.

Example
~~~~~~~

::

  AT%XSYSTEMMODE=1,0,1,0

  OK
  AT%XCOEX0=1,1,1565,1586

  OK
  AT+CPSMS=1

  OK
  AT+CFUN=1

  OK
  AT#XNRFCLOUD=1

  OK
  #XNRFCLOUD: 1,0
  AT#XAGPS=1,1

  #XAGPS: 1,1

  OK

  #XGPS: 35.457417,139.625211,162.850952,15.621976,1.418092,0.000000,"2021-06-02 05:21:31"

  #XGPS: 35.457435,139.625348,176.104797,14.245458,1.598184,69.148659,"2021-06-02 05:21:32"

  #XGPS: 35.457417,139.625415,179.132980,13.318132,1.235241,69.148659,"2021-06-02 05:21:33"

  #XGPS: 35.457410,139.625469,181.223541,12.667312,0.803951,69.148659,"2021-06-02 05:21:34"

Read command
------------

The read command allows you to check GNSS support and AGPS service status.

Syntax
~~~~~~

::

   #XAGPS?

Response syntax
~~~~~~~~~~~~~~~

::

   #XAGPS: <gnss_service>,<agps_status>

* The ``<gnss_service>`` value is an integer.
  When it returns the value of ``1``, it means that GNSS is supported in ``%XSYSTEMMODE`` and activated in ``+CFUN``.

* The ``<agps_status>`` value is an integer.

* ``0`` - AGPS is stopped.
* ``1`` - AGPS is started.
* ``2`` - GNSS wakes up in periodic mode.
* ``3`` - GNSS enters sleep because of timeout.
* ``4`` - GNSS enters sleep because a fix is achieved.

Example
~~~~~~~

::

  AT#XAGPS?

  #XAGPS: 1,1

  OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XAGPS=?

Example
~~~~~~~

::

  AT#XAGPS=?

  #XAGPS: (0,1),<interval>,<timeout>

  OK


Run GNSS with nRF Cloud P-GPS
=============================

The ``#XPGPS`` command runs the GNSS together with the nRF Cloud P-GPS service.

.. note::
   To use ``#XPGPS``, the following preconditions apply:

   * You must define :ref:`CONFIG_SLM_PGPS <CONFIG_SLM_PGPS>`.
   * You must have access to nRF Cloud through the LTE network for receiving P-GPS data.

Set command
-----------

The set command allows you to start and stop the GNSS together with the nRF Cloud P-GPS service.

Syntax
~~~~~~

::

   #XPGPS=<op>[,<interval>[,<timeout>]]

The ``<op>`` parameter accepts the following integer values:

* ``0`` - Stop GNSS with P-GPS
* ``1`` - Start GNSS with P-GPS

The ``<interval>`` parameter represents the GNSS fix interval in seconds.
It must be set when starting the GNSS.
It accepts the following integer values:

* Ranging from ``10`` to ``1800`` - Periodic navigation mode.
  The fix interval is set to the specified value.

In periodic navigation mode, the ``<timeout>`` parameter controls the maximum time in seconds that the GNSS receiver is allowed to run while trying to produce a valid PVT estimate.
In continuous navigation mode, this parameter does not have any effect.
It accepts the following integer values:

* ``0`` - The GNSS receiver runs indefinitely until a valid PVT estimate is produced.
* Any positive integer lower than the ``<interval>`` value - The GNSS receiver is turned off after the specified time is up, even if a valid PVT estimate was not produced.

When not specified, it defaults to a timeout value of 60 seconds.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XGPS: <latitude>,<longitude>,<altitude>,<accuracy>,<speed>,<heading>,<datetime>

* The ``<latitude>`` value represents the latitude in degrees.
* The ``<longitude>`` value represents the longitude in degrees.
* The ``<altitude>`` value represents the altitude above the WGS-84 ellipsoid in meters.
* The ``<accuracy>`` value represents the accuracy (2D 1-sigma) in meters.
* The ``<speed>`` value represents the horizontal speed in meters.
* The ``<heading>`` value represents the heading of the movement of the user in degrees.
* The ``<datetime>`` value represents the UTC date-time.

::

   #XGPS: <NMEA message>

The ``<NMEA message>`` is the ``$GPGGA`` (Global Positioning System Fix Data) NMEA sentence.

::

   #XPGPS: <gnss_service>,<pgps_status>

Refer to the READ command.

Example
~~~~~~~

::

  AT%XSYSTEMMODE=1,0,1,0

  OK
  AT%XCOEX0=1,1,1565,1586

  OK
  AT+CPSMS=1

  OK
  AT+CFUN=1

  OK
  AT#XNRFCLOUD=1

  OK
  #XNRFCLOUD: 1,0
  AT#XPGPS=1,30

  #XPGPS: 1,1

  OK

  #XGPS: 35.457243,139.625435,149.005020,28.184258,10.431827,281.446014,"2021-06-24 04:35:52"

  #XGPS: 35.457189,139.625602,176.811203,43.015198,0.601837,281.446014,"2021-06-24 04:36:28"

  #XGPS: 35.457498,139.625422,168.243591,31.753956,0.191195,281.446014,"2021-06-24 04:36:41"

  #XGPS: 35.457524,139.624667,100.745979,25.324850,6.347160,94.699837,"2021-06-24 04:37:10"

Read command
------------

The read command allows you to check GNSS support and PGPS service status.

Syntax
~~~~~~

::

   #XPGPS?

Response syntax
~~~~~~~~~~~~~~~

::

   #XPGPS: <gnss_service>,<pgps_status>

* The ``<gnss_service>`` value is an integer.
  When it returns the value of ``1``, it means that GNSS is supported in ``%XSYSTEMMODE`` and is activated in ``+CFUN``.

* The ``<pgps_status>`` value is an integer.

* ``0`` - PGPS is stopped.
* ``1`` - PGPS is started.
* ``2`` - GNSS wakes up in periodic mode.
* ``3`` - GNSS enters sleep because of timeout.
* ``4`` - GNSS enters sleep because a fix is achieved.

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XPGPS=?

Example
~~~~~~~

::

  AT#XPGPS=?

  #XPGPS: (0,1),<interval>,<timeout>

  OK

Delete GNSS data
================

The ``#XGPSDEL`` command deletes GNSS data from non-volatile memory.
This command should be issued when GNSS is activated but not started yet.

Set command
-----------

The set command allows you to delete old GNSS data.

Syntax
~~~~~~

::

   #XGPSDEL=<mask>

The ``<mask>`` parameter accepts an integer that is the ``OR`` value of the following bitmasks :

* ``0x001`` - Ephemerides
* ``0x002`` - Almanacs (excluding leap second and ionospheric correction)
* ``0x004`` - Ionospheric correction parameters
* ``0x008`` - Last good fix (the last position)
* ``0x010`` - GPS time-of-week (TOW)
* ``0x020`` - GPS week number
* ``0x040`` - Leap second (UTC parameters)
* ``0x080`` - Local clock (TCXO) frequency offset
* ``0x100`` - Precision estimate of GPS time-of-week (TOW)
* ``511`` - All of the above

Example
~~~~~~~

::

  AT%XSYSTEMMODE=0,0,1,0
  OK
  AT+CFUN=31
  OK
  AT#XGPSDEL=511
  OK
  AT+CFUN=0
  OK

Read command
------------

The read command is not supported.

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XGPSDEL=?

Example
~~~~~~~

::

  AT#XGPSDEL=?

  #XGPSDEL: <mask>

  OK

Run nRF Cloud cellular positioning
==================================

The ``#XCELLPOS`` command runs the nRF Cloud cellular positioning service for position information.

.. note::
   To use ``#XCELLPOS``, the following preconditions apply:

   * You must define :ref:`CONFIG_SLM_CELL_POS <CONFIG_SLM_CELL_POS>`.
   * You must have access to nRF Cloud through the LTE network.

Set command
-----------

The set command allows you to start and stop the nRF Cloud cellular positioning service.

Syntax
~~~~~~

::

   #XCELLPOS=<op>

The ``<op>`` parameter accepts the following integer values:

* ``0`` - Stop cellular positioning.
* ``1`` - Start cellular positioning in single-cell mode.
* ``2`` - Start cellular positioning in multi-cell mode.
  To use ``2``, you must issue the ``AT%NCELLMEAS`` command first.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XCELLPOS: <type>,<latitude>,<longitude>,<uncertainty>

* The ``<type>`` value indicates in which mode the cellular positioning server is running:

  * ``0`` - The server is running in single-cell mode
  * ``1`` - The server is running in multi-cell mode

* The ``<latitude>`` value represents the latitude in degrees.
* The ``<longitude>`` value represents the longitude in degrees.
* The ``<uncertainty>`` value represents the certainty of the result.

Example
~~~~~~~

::

  AT%XSYSTEMMODE=1,0,0,0

  OK
  AT+CFUN=1

  OK
  AT#XNRFCLOUD=1

  OK
  #XNRFCLOUD: 1,0
  AT#XCELLPOS=1

  OK

  #XCELLPOS: 0,35.455833,139.626111,1094

  AT%NCELLMEAS

  OK

  %NCELLMEAS: 0,"0199F10A","44020","107E",65535,3750,5,49,27,107504,3750,251,33,4,0,475,107,26,14,25,475,58,26,17,25,475,277,24,9,25,475,51,18,1,25

  AT#XCELLPOS=2

  OK

  #XCELLPOS: 1,35.534999,139.722362,1801
  AT#XCELLPOS=0

  OK

Read command
------------

The read command allows you to check GNSS support and CELLPOS service status.

Syntax
~~~~~~

::

   #XCELLPOS?

Response syntax
~~~~~~~~~~~~~~~

::

   #XCELLPOS: <gnss_service>,<cellpos_status>

* The ``<gnss_service>`` value is an integer.
  When it returns the value of ``1``, it means that GNSS is supported in ``%XSYSTEMMODE`` and is activated in ``+CFUN``.

.. note::
   CELLPOS does not require the GNSS service in modem.

* The ``<cellpos_status>`` value is an integer.
  When it returns the value of ``1``, it means that CELLPOS is started.

Example
~~~~~~~

::

  AT#XAGPS?

  #XAGPS: 1,1

  OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XCELLPOS=?

Example
~~~~~~~

::

  AT#XCELLPOS=?

  #XCELLPOS: (0,1,2)

  OK
