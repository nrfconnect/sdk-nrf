.. _SLM_AT_GNSS:

GNSS AT commands
****************

.. contents::
   :local:
   :depth: 2

The following list contains GNSS-related AT commands.

Control GNSS
============

The ``#XGPS`` command controls the GNSS.

Set command
-----------

The set command allows you to start and stop the GNSS module.

Both the :ref:`lib_nrf_cloud_agnss` service and the :ref:`lib_nrf_cloud_pgps` service can be used with the module, either together or independently of each other.
Using them reduces the time it takes the GNSS module to estimate its position.

To use either of them, the device must be connected to nRF Cloud (using the :ref:`#XNRFCLOUD <SLM_AT_NRFCLOUD>` AT command) when starting the GNSS module.
In addition, the following Kconfig options must be enabled:

   * :kconfig:option:`CONFIG_NRF_CLOUD_AGNSS <CONFIG_NRF_CLOUD_AGNSS>` to use A-GNSS.
   * :kconfig:option:`CONFIG_NRF_CLOUD_PGPS <CONFIG_NRF_CLOUD_PGPS>` to use P-GPS.

If both assistive services were enabled during compilation, you cannot choose to use only one of them at run time.

Syntax
~~~~~~

::

   #XGPS=<op>,<cloud_assistance>,<interval>[,<timeout>]

The ``<op>`` parameter can have the following integer values:

* ``0`` - Stop the GNSS module.
  In this case, no other parameter is allowed.
* ``1`` - Start the GNSS module.

``<cloud_assistance>`` is an integer that indicates whether to use the nRF Cloud assistive services that were enabled during compilation.
It is ``0`` for disabled or ``1`` for enabled.

``<interval>`` is an integer that indicates the GNSS fix interval in seconds.
It can have one of the following values:

* ``0`` - Single-fix navigation mode.
* ``1`` - Continuous navigation mode.
  The fix interval is set to 1 second.
* Ranging from ``10`` to ``65535`` - Periodic navigation mode.
  The fix interval is set to the specified value.

In continuous navigation mode, the ``<timeout>`` parameter must be omitted.
In single-fix and periodic navigation modes, the ``<timeout>`` parameter indicates the maximum time in seconds that the GNSS receiver is allowed to run while trying to produce a valid Position, Velocity, and Time (PVT) estimate.
It can be one of the following:

* ``0`` - The GNSS receiver runs indefinitely until a valid PVT estimate is produced.
* Any positive integer - The GNSS receiver is turned off after the specified time is up, even if a valid PVT estimate was not produced.
* Omitted - In single-fix or periodic navigation mode, the timeout defaults to 60 seconds.

.. note::

   When ``<cloud_assistance>`` is disabled, no request is made to nRF Cloud for assistance data.
   However, if it has been previously enabled and used, such data may remain locally and will be used if still valid.

.. note::

   In periodic navigation mode, the ``<interval>`` and ``<timeout>`` parameters are temporarily ignored during the first fix (for up to 60 seconds).

.. note::

   Make sure that the GNSS antenna is configured properly.
   This can be achieved two ways:

   * Using the :kconfig:option:`CONFIG_MODEM_ANTENNA_AT_MAGPIO` and :kconfig:option:`CONFIG_MODEM_ANTENNA_AT_COEX0` Kconfig options.
   * By issuing the ``%XMAGPIO`` and ``%XCOEX0`` AT commands manually at run time.

.. tip::

   When the LTE link is enabled, make sure to have either Power Saving Mode (PSM) or extended Discontinuous Reception (eDRX) enabled to give the GNSS receiver the time it needs to acquire fixes.

.. tip::

   Enable the :kconfig:option:`CONFIG_SLM_LOG_LEVEL_DBG` Kconfig option if you have trouble acquiring fixes.
   It makes the application print NMEA and PVT data when trying to acquire fixes, which can be of help when solving the issue.

.. note::

   See the documentation for the :ref:`lib_nrf_cloud_agnss` and :ref:`lib_nrf_cloud_pgps` libraries for information on how to best configure and use A-GNSS and P-GPS, respectively.

.. note::

   When using P-GPS, make sure that the value of the :kconfig:option:`CONFIG_SLM_PGPS_INJECT_FIX_DATA` Kconfig option matches your use case.
   It is enabled by default but should be disabled if the device is expected to move distances longer than a few dozen kilometers between fix attempts.

As an alternative to GNSS-based positioning, see :ref:`#XNRFCLOUDPOS <SLM_AT_NRFCLOUDPOS>` for cellular and Wi-Fi positioning.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block::

   #XGPS: <latitude>,<longitude>,<altitude>,<accuracy>,<speed>,<heading>,<datetime>

* The ``<latitude>`` value represents the latitude in degrees.
* The ``<longitude>`` value represents the longitude in degrees.
* The ``<altitude>`` value represents the altitude above the WGS-84 ellipsoid in meters.
* The ``<accuracy>`` value represents the accuracy (2D 1-sigma) in meters.
* The ``<speed>`` value represents the horizontal speed in meters.
* The ``<heading>`` value represents the heading of the movement of the user in degrees.
* The ``<datetime>`` value represents the UTC date-time.

.. gps_status_notif_start

.. code-block::

   #XGPS: <gnss_service>,<gnss_status>

* The ``<gnss_service>`` parameter is an integer.
  When it has the value ``1``, it means that GNSS is supported in ``%XSYSTEMMODE`` and activated in ``+CFUN``.

* The ``<gnss_status>`` parameter is an integer.
  It can have the following values:

  * ``0`` - GNSS is stopped.
  * ``1`` - GNSS is started.
  * ``2`` - GNSS wakes up in periodic mode.
  * ``3`` - GNSS enters sleep because of timeout.
  * ``4`` - GNSS enters sleep because a fix is acquired.

.. gps_status_notif_end

Example
~~~~~~~

::

  AT%XSYSTEMMODE=0,0,1,0

  OK
  AT+CFUN=31

  OK
  AT#XGPS=1,0,0,0,0

  #XGPS: 1,1

  OK

  #XGPS: 1,4

  #XGPS: 35.457576,139.625090,121.473785,22.199919,0.442868,0.000000,"2021-06-02 06:25:48"

::

  AT%XSYSTEMMODE=1,0,1,0

  OK
  AT+CPSMS=1,,,"00000001","00000011"

  OK
  AT+CEDRXS=2,4,"0011"

  OK
  AT+CFUN=1

  OK

  +CEDRXP: 4,"0011","0011","0011"

  AT#XNRFCLOUD=1

  OK
  #XNRFCLOUD: 1,0
  AT#XGPS=1,1,0,1

  #XGPS: 1,1

  OK

  #XGPS: 35.457417,139.625211,162.850952,15.621976,1.418092,0.000000,"2021-06-02 05:21:31"

  #XGPS: 35.457435,139.625348,176.104797,14.245458,1.598184,69.148659,"2021-06-02 05:21:32"

  #XGPS: 35.457417,139.625415,179.132980,13.318132,1.235241,69.148659,"2021-06-02 05:21:33"

  #XGPS: 35.457410,139.625469,181.223541,12.667312,0.803951,69.148659,"2021-06-02 05:21:34"
  AT#XGPS=0

  #XGPS: 1,0

  OK

::

  AT%XSYSTEMMODE=1,0,1,0

  OK
  AT+CPSMS=1,,,"00000001","00000011"

  OK
  AT+CEDRXS=2,4,"0011"

  OK
  AT+CFUN=1

  OK
  AT#XNRFCLOUD=1

  OK
  #XNRFCLOUD: 1,0
  AT#XGPS=1,0,1,30

  #XGPS: 1,1

  OK

  #XGPS: 1,2

  #XGPS: 1,4

  #XGPS: 35.457243,139.625435,149.005020,28.184258,10.431827,281.446014,"2021-06-24 04:35:52"

  #XGPS: 1,2

  #XGPS: 1,4

  #XGPS: 35.457189,139.625602,176.811203,43.015198,0.601837,281.446014,"2021-06-24 04:36:32"

  #XGPS: 1,2

  #XGPS: 1,4

  #XGPS: 35.457498,139.625422,168.243591,31.753956,0.191195,281.446014,"2021-06-24 04:37:12"

  #XGPS: 1,2

  #XGPS: 1,4

  #XGPS: 35.457524,139.624667,100.745979,25.324850,6.347160,94.699837,"2021-06-24 04:37:52"
  AT#XGPS=0

  #XGPS: 1,0

  OK

Read command
------------

The read command allows you to check GNSS support and its status.

Syntax
~~~~~~

::

   #XGPS?

Response syntax
~~~~~~~~~~~~~~~

.. include:: GNSS_AT_commands.rst
   :start-after: gps_status_notif_start
   :end-before: gps_status_notif_end

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

  #XGPS: (0,1),(0,1),<interval>,<timeout>
  OK

Delete GNSS data
================

The ``#XGPSDEL`` command deletes GNSS data from non-volatile memory.
This command should be issued when GNSS is activated but not started yet.

.. note::
   This is a debug feature, and is not supposed to be used in production code.

Set command
-----------

The set command allows you to delete cached GNSS data.
Using this command does not trigger A-GNSS nor P-GPS data request events.
The execution of this command may delay the full functionality of A-GNSS and P-GPS until the next periodic data request has been received.

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
* ``0x100`` - Precision estimate of GPS time-of-week (TOW)
* ``383`` - All of the above

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
