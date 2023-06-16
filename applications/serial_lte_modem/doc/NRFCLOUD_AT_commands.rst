.. _SLM_AT_NRFCLOUD:

nRF Cloud AT commands
*********************

.. contents::
   :local:
   :depth: 2

The following commands list contains nRF Cloud-related AT commands.

nRF Cloud access
================

The ``#XNRFCLOUD`` command controls the access to the nRF Cloud service.

.. note::
   To use ``#XNRFCLOUD``, the following preconditions apply:

   * You must first provision the device to nRF Cloud, using the UUID from the modem firmware as device ID.
   * You must define :ref:`CONFIG_SLM_NRF_CLOUD <CONFIG_SLM_NRF_CLOUD>`.
   * You must have access to nRF Cloud through the LTE network.

Set command
-----------

The set command allows you to access the nRF Cloud service.

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

.. note::
   The application supports nRF Cloud cloud2device appId ``MODEM`` to send AT command from cloud:

   * cloud2device schema::

       {"appId":"MODEM", "messageType":"CMD", "data":"<AT_command>"}.

   * device2cloud schema::

       {"appId":"MODEM", "messageType":"RSP", "data":"<AT_response>"}.

   The application executes the AT command in a best-effort way.

.. note::
   The application supports nRF Cloud cloud2device appId ``DEVICE`` to gracefully disconnect from cloud:

   * cloud2device schema::

       {"appId":"DEVICE", "messageType":"DISCON"}.

   There is no response sending to nRF Cloud for this appId.

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
  OK
  {"msg":"Hello, nRF Cloud"}+++
  #XDATAMODE: 0

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

nRF Cloud cellular location
===========================

The ``#XCELLPOS`` command runs the nRF Cloud cellular location service for location information.

.. note::
   To use ``#XCELLPOS``, the following preconditions apply:

   * You must define :ref:`CONFIG_SLM_NRF_CLOUD <CONFIG_SLM_NRF_CLOUD>` and :kconfig:option:`CONFIG_NRF_CLOUD_LOCATION <CONFIG_NRF_CLOUD_LOCATION>`.
   * You must have access to nRF Cloud through the LTE network.

Set command
-----------

The set command allows you to start and stop the nRF Cloud cellular location service.

Syntax
~~~~~~

::

   #XCELLPOS=<op>

The ``<op>`` parameter accepts the following integer values:

* ``0`` - Stop cellular location.
* ``1`` - Start cellular location in single-cell mode.
* ``2`` - Start cellular location in multi-cell mode.
  To use ``2``, you must issue the ``AT%NCELLMEAS`` command first.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XCELLPOS: <type>,<latitude>,<longitude>,<uncertainty>

* The ``<type>`` value indicates in which mode the cellular location service is running:

  * ``0`` - The service is running in single-cell mode
  * ``1`` - The service is running in multi-cell mode

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

The read command allows you to check the cellular location service status.

Syntax
~~~~~~

::

   #XCELLPOS?

Response syntax
~~~~~~~~~~~~~~~

::

   #XCELLPOS: <cellpos_status>

* The ``<cellpos_status>`` value is an integer.
  When it returns the value of ``1``, it means that the cellular location service is started.

Example
~~~~~~~

::

  AT#XCELLPOS?

  #XCELLPOS: 1

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

nRF Cloud Wi-Fi location
========================

The ``#XWIFIPOS`` command runs the nRF Cloud Wi-Fi location service to receive location information.

.. note::
   To use ``#XWIFIPOS``, the following preconditions apply:

   * You must define :ref:`CONFIG_SLM_NRF_CLOUD <CONFIG_SLM_NRF_CLOUD>` and :kconfig:option:`CONFIG_NRF_CLOUD_LOCATION <CONFIG_NRF_CLOUD_LOCATION>`.
   * You must have access to nRF Cloud through the LTE network.

Set command
-----------

The set command allows you to start and stop the nRF Cloud Wi-Fi location service.

Syntax
~~~~~~

::

   #XWIFIPOS=<op>[,<ssid0>,<mac0>[,<ssid1>,<mac1>[...]]]

The ``<op>`` parameter accepts the following integer values:

* ``0`` - Stop Wi-Fi location.
* ``1`` - Start Wi-Fi location.

* The ``<ssidX>`` parameter is a string.
  It indicates the SSID of the Wi-Fi access point.

* The ``<macX>`` parameter is a string.
  It indicates the MAC address of the Wi-Fi access point and should be formatted as ``%02x:%02x:%02x:%02x:%02x:%02x``.

The command accepts the ``<ssidX>`` and ``<macX>`` values of up to 5 access points.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XWIFIPOS: <type>,<latitude>,<longitude>,<uncertainty>

* The ``<type>`` value indicates in which mode the Wi-Fi location service is running:

  * ``2`` - The service is running in Wi-Fi mode

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
  AT#XWIFIPOS=1,"Nordic_WLAN_5GHz","40:9b:cd:c1:5a:40","Nordic_Guest","00:90:fe:eb:4f:42"

  OK

  #XWIFIPOS: 2,35.457272,139.624395,60
  AT#XWIFIPOS=0

  OK

Read command
------------

The read command allows you to check the Wi-Fi location service status.

Syntax
~~~~~~

::

   #XWIFIPOS?

Response syntax
~~~~~~~~~~~~~~~

::

   #XWIFIPOS: <wifipos_status>

* The ``<wifipos_status>`` value is an integer.
  When it returns the value of ``1``, it means that the Wi-Fi location service is started.

Example
~~~~~~~

::

  AT#XWIFIPOS?

  #XWIFIPOS: 0

  OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XWIFIPOS=?

Example
~~~~~~~

::

  AT#XWIFIPOS=?

  #XWIFIPOS: (0,1)

  OK
