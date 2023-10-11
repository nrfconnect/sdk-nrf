nRF Cloud AT commands
*********************

.. contents::
   :local:
   :depth: 2

The following list contains nRF Cloud-related AT commands.

.. _SLM_AT_NRFCLOUD:

nRF Cloud access
================

The ``#XNRFCLOUD`` command controls the access to the nRF Cloud service.

.. note::
   To use ``#XNRFCLOUD``, the following preconditions apply:

   * You must first preconnect provision the device to nRF Cloud, using the UUID from the modem firmware as device ID.
     See `nRF Cloud Preconnect Provisioning`_ for more information.
   * The :ref:`CONFIG_SLM_NRF_CLOUD <CONFIG_SLM_NRF_CLOUD>` Kconfig option must be enabled.
   * The device must have access to nRF Cloud through the LTE network.

Set command
-----------

The set command allows you to access the nRF Cloud service.

Syntax
~~~~~~

::

   #XNRFCLOUD=<op>[,<send_location>]

The ``<op>`` parameter can have the following integer values:

* ``0`` - Disconnect from the nRF Cloud service.
* ``1`` - Connect to the nRF Cloud service.
* ``2`` - Send a message in the JSON format to the nRF Cloud service.

When ``<op>`` is ``2``, SLM enters ``slm_data_mode``.

The ``<send_location>`` parameter is used only when the value of ``<op>`` is ``1``.
It can have the following integer values:

* ``0`` - The device location is not sent to nRF Cloud.
  This is the default behavior if the parameter is omitted.
* ``1`` - The device location is sent to nRF Cloud.

.. note::
   Sending the device location to nRF Cloud is done in a best-effort way.
   For it to happen, the device must have :ref:`GNSS <SLM_AT_GNSS>` enabled and it must be acquiring fixes.
   The minimal report interval is 5 seconds.

.. note::
   The application supports the nRF Cloud cloud2device appId ``MODEM`` to send an AT command from the cloud:

   * cloud2device schema::

       {"appId":"MODEM", "messageType":"CMD", "data":"<AT_command>"}.

   * device2cloud schema::

       {"appId":"MODEM", "messageType":"RSP", "data":"<AT_response>"}.

   The application executes the AT command in a best-effort way.

.. note::
   The application supports the nRF Cloud cloud2device appId ``DEVICE`` to gracefully disconnect from the cloud:

   * cloud2device schema::

       {"appId":"DEVICE", "messageType":"DISCON"}.

   There is no response sent to nRF Cloud for this appId.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XNRFCLOUD: <ready>,<send_location>

* The ``<ready>`` value indicates whether the connection to nRF Cloud is established or not.
* The ``<send_location>`` value indicates whether the device location will be sent to nRF Cloud or not.

::

   #XNRFCLOUD: <message>

* The ``<message>`` value indicates data received from nRF Cloud that is not a supported cloud2device appId.

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

The read command checks whether the connection to nRF Cloud is established or not.

Syntax
~~~~~~

::

   #XNRFCLOUD?

Response syntax
~~~~~~~~~~~~~~~

::

   #XNRFCLOUD: <ready>,<send_location>,<sec_tag>,<device_id>

* The ``<ready>`` value indicates whether the connection to nRF Cloud is established or not.
* The ``<send_location>`` value indicates whether the device location will be sent to nRF Cloud or not.
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

  AT#XNRFCLOUD=?

  #XNRFCLOUD: (0,1,2),<send_location>

  OK

.. _SLM_AT_NRFCLOUDPOS:

nRF Cloud location
==================

The ``#XNRFCLOUDPOS`` command sends a request to nRF Cloud to determine the device's location.
The request uses information from the cellular network, Wi-Fi access points, or both.

.. note::
   To use ``#XNRFCLOUDPOS``, the following preconditions apply:

   * The device must be connected to nRF Cloud using :ref:`#XNRFCLOUD <SLM_AT_NRFCLOUD>`.
   * The :kconfig:option:`CONFIG_NRF_CLOUD_LOCATION <CONFIG_NRF_CLOUD_LOCATION>` Kconfig option must be enabled.

Set command
-----------

The set command allows sending a location request to nRF Cloud.

Syntax
~~~~~~

::

   #XNRFCLOUDPOS=<cell_pos>,<wifi_pos>[,<MAC 1>[,<RSSI 1>],<MAC 2>[,<RSSI 2>][,<MAC 3>[...]]]

The ``<cell_pos>`` parameter can have the following integer values:

* ``0`` - Do not include cellular network information in the location request.
* ``1`` - Use single-cell cellular network information (only the serving cell).
* ``2`` - Use multi-cell cellular network information (the serving and possibly neighboring cells).
  To use this option, you must first issue the ``AT%NCELLMEAS`` command and wait for its result notification.

  The cellular network information included in the location request will be the one received from the ``AT%NCELLMEAS`` command.
  This means that, for the most up-to-date location information, you should use the command as close to sending the location request as possible.
  Also, keep in mind that whenever you send a location request in single-cell mode, any previously saved multi-cell cellular network information is invalidated.

The ``<wifi_pos>`` parameter can have the following integer values:

* ``0`` - Do not include Wi-Fi access point information in the location request.
* ``1`` - Use Wi-Fi access point information.
  The access points must be given as additional parameters to the command.
  The minimum number of access points to provide is two (:c:macro:`NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN`), and the maximum is limited by the AT command parameter count limit (:ref:`CONFIG_SLM_AT_MAX_PARAM <CONFIG_SLM_AT_MAX_PARAM>`).

The ``<MAC x>`` parameter is a string.
It indicates the MAC address of a Wi-Fi access point and must be formatted as ``%02x:%02x:%02x:%02x:%02x:%02x`` (:c:macro:`WIFI_MAC_ADDR_TEMPLATE`).

The ``<RSSI x>`` parameter is an optional integer.
It indicates the signal strength of a Wi-Fi access point in dBm, between ``-128`` and ``0``.
If provided, it must follow the MAC address parameter of the access point.
Providing the RSSI parameters helps improve the accuracy of the Wi-Fi location.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XNRFCLOUDPOS: <error>

This is emitted when the location request failed, either when sending it or receiving its response.
No notification containing location data will be emitted.

* The ``<error>`` value indicates the error that happened.
  It is either a negative *errno* code or one of the :c:enum:`nrf_cloud_error` values.

::

   #XNRFCLOUDPOS: <type>,<latitude>,<longitude>,<uncertainty>

This is emitted when a successful response to a sent location request is received.

* The ``<type>`` value indicates the service used to fulfill the location request.

  * ``0`` (:c:enumerator:`LOCATION_TYPE_SINGLE_CELL`) - Single-cell cellular location.
  * ``1`` (:c:enumerator:`LOCATION_TYPE_MULTI_CELL`) - Multi-cell cellular location.
  * ``2`` (:c:enumerator:`LOCATION_TYPE_WIFI`) - Wi-Fi location.

* The ``<latitude>`` value represents the latitude in degrees.
* The ``<longitude>`` value represents the longitude in degrees.
* The ``<uncertainty>`` value represents the radius of the uncertainty circle around the location in meters, also known as Horizontal Positioning Error (HPE).

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
  AT#XNRFCLOUDPOS=1,0

  OK

  #XNRFCLOUDPOS: 0,35.455833,139.626111,1094
  AT%NCELLMEAS

  OK

  %NCELLMEAS: 0,"0199F10A","44020","107E",65535,3750,5,49,27,107504,3750,251,33,4,0,475,107,26,14,25,475,58,26,17,25,475,277,24,9,25,475,51,18,1,25
  AT#XNRFCLOUDPOS=2,0

  OK

  #XNRFCLOUDPOS: 1,35.455833,139.626111,1094
  AT#XNRFCLOUDPOS=0,1,"40:9b:cd:c1:5a:40","00:90:fe:eb:4f:42"

  OK

  #XNRFCLOUDPOS: 2,35.457335,139.624443,60
  AT#XNRFCLOUDPOS=0,1,"40:9b:cd:c1:5a:40",-40,"00:90:fe:eb:4f:42",-69

  OK

  #XNRFCLOUDPOS: 2,35.457346,139.624449,20

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
