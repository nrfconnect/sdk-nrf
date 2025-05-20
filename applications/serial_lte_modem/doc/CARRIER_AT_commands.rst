.. _SLM_AT_CARRIER:

LwM2M carrier library AT commands
*********************************

.. contents::
   :local:
   :depth: 2

This page describes AT commands related to the LwM2M carrier library.

Carrier event #XCARRIEREVT
==========================

The ``#XCARRIEREVT`` is an unsolicited notification that indicates the event of the LwM2M carrier library.

Unsolicited notification
------------------------

It indicates the event of the LwM2M carrier library.

Syntax
~~~~~~

::

   #XCARRIEREVT: <evt_type>,<param1>[,<param2>[,<param3>]..]

The ``<evt_type>`` value is an integer indicating the type of the event, followed by a variable number of parameters depending on the event type.
These may be the following:

* ``#XCARRIEREVT: 1,<status>``

  Request to use the ``AT+CFUN=1`` AT command to set the modem to full functional mode.
  ``<status>`` returns two possible values:

  * ``0`` - Request handling is fulfilled automatically.
  * ``-1`` - Request handling is deferred and must be fulfilled by the application at its earliest convenience.

* ``#XCARRIEREVT: 2,<status>``

  Request to use the ``AT+CFUN=4`` AT command to set the modem to flight functional mode.
  ``<status>`` returns two possible values:

  * ``0`` - Request handling is fulfilled automatically.
  * ``-1`` - Request handling is deferred and must be fulfilled by the application at its earliest convenience.

* ``#XCARRIEREVT: 3,<status>``

  Request to use the ``AT+CFUN=0`` AT command to set the modem to minimum functional mode.
  ``<status>`` returns two possible values:

  * ``0`` - Request handling is fulfilled automatically.
  * ``-1`` - Request handling is deferred and must be fulfilled by the application at its earliest convenience.

* ``#XCARRIEREVT: 4,0``

  Bootstrap sequence complete.

* ``#XCARRIEREVT: 5,0``

  Device registered successfully to the device management servers.

* ``#XCARRIEREVT: 6,0``

  Device deregistered successfully from the device management servers.

* ``#XCARRIEREVT: 7,<reason>,<timeout>``

  Client operation has been deferred.

  ``<reason>`` indicates the reason for the connection deferral and can return the following values:

  * ``0`` - - :c:macro:`LWM2M_CARRIER_DEFERRED_NO_REASON`.
  * ``1`` - - :c:macro:`LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE`.
  * ``2`` - - :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE`.
  * ``3`` - - :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT`.
  * ``4`` - - :c:macro:`LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE`.
  * ``5`` - - :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE`.
  * ``6`` - - :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_CONNECT`.
  * ``7`` - - :c:macro:`LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION`.
  * ``8`` - - :c:macro:`LWM2M_CARRIER_DEFERRED_SERVICE_UNAVAILABLE`.
  * ``9`` - - :c:macro:`LWM2M_CARRIER_DEFERRED_SIM_MSISDN`.

  ``<timeout>`` indicates the time in seconds before the operation is resumed.

* ``#XCARRIEREVT: 8,0``

  Firmware update started.

* ``#XCARRIEREVT: 9,0``

  Firmware updated successfully.

* ``#XCARRIEREVT: 10,<status>``

  Request to perform an application reboot, for example using the ``AT#XRESET`` AT command.

  ``<status>`` returns two possible values:

  * ``0`` - Request handling is fulfilled automatically.
  * ``-1`` - Request handling is deferred and must be fulfilled by the application at its earliest convenience.

* ``#XCARRIEREVT: 11,0``

  Modem domain event received.

* ``#XCARRIEREVT: 12,<type>,<URI>[,<data_length>\r\n<data>]``

  Operation performed on the Binary App Data Container object (ID:19) or the App Data Container object (ID: 10250).

  ``<type>`` indicates the type of operation performed by the device management server:

  * ``0`` - A write request.
  * ``1`` - An observation start request.
  * ``2`` - An observation stop request.

  ``<URI>`` is a plain-text string in double quotes that describes the URI path that was targeted by the operation.

  ``<data_length>`` and ``<data>`` parameters are only applicable to notifications of ``<type>`` 0 (write).

  ``<data_length>`` indicates the length in bytes of ``<data>``.

  ``<data>`` is a hexadecimal string in double quotes that contains the data written by the device management server to the indicated ``<URI>`` path.

* ``#XCARRIEREVT: 13,0``

  Request to initialize the modem.

* ``#XCARRIEREVT: 14,0``

  Request to shut down the modem.

* ``#XCARRIEREVT: 15,0``

  The device error codes have been reset by the server.

* ``#XCARRIEREVT: 20,<type>,<value>``

  LwM2M carrier library error occurred.

  ``<type>`` indicates the type of error and can return the following values:

  * ``0`` - - :c:macro:`LWM2M_CARRIER_ERROR_NO_ERROR`.
  * ``1`` - - :c:macro:`LWM2M_CARRIER_ERROR_LTE_LINK_UP_FAIL`.
  * ``2`` - - :c:macro:`LWM2M_CARRIER_ERROR_LTE_LINK_DOWN_FAIL`.
  * ``3`` - - :c:macro:`LWM2M_CARRIER_ERROR_BOOTSTRAP`.
  * ``4`` - - :c:macro:`LWM2M_CARRIER_ERROR_FOTA_FAIL`.
  * ``5`` - - :c:macro:`LWM2M_CARRIER_ERROR_CONFIGURATION`.
  * ``6`` - - :c:macro:`LWM2M_CARRIER_ERROR_INIT`.
  * ``8`` - - :c:macro:`LWM2M_CARRIER_ERROR_CONNECT`.

  ``<value>`` indicates the error value returned in this event.

LwM2M Carrier library #XCARRIER
===============================

The ``#XCARRIER`` command allows you to send LwM2M carrier library commands.

Set command
-----------

The set command allows you to send LwM2M carrier library commands.

Syntax
~~~~~~

::

   AT#XCARRIER=<cmd>[,<param1>[,<param2>]..]

The ``<cmd>`` command is a string, and can be used as follows:

* ``AT#XCARRIER="app_data_create",<obj_inst_id>,<res_inst_id>``

  Create an empty resource instance of the Data resource (ID: 0) of the Binary App Data Container object (ID: 19).

  ``<obj_inst_id>`` indicates the target object instance.

  ``<res_inst_id>`` indicates the target resource instance.

* ``AT#XCARRIER="app_data_set"[,<data>][,<obj_inst_id>,<res_inst_id>]``

  Put the value in ``<data>`` into the indicated path.
  ``<data>`` must be a hexadecimal string in double quotes, unless ``slm_data_mode`` is enabled.

  * If ``<obj_inst_id>`` and ``<res_inst_id>`` are specified, the data is set in an instance of the Data resource (ID: 0) of the Binary App Data Container object (ID: 19).
    The URI path of the resource instance is indicated as ``/19/<obj_inst_id>/0/<res_inst_id>``.
  * If ``<obj_inst_id>`` and ``<res_inst_id>`` are not present, the data is set in the Uplink Data resource (ID: 0) of the App Data Container object (ID: 10250).
    The URI path of the resource instance is indicated as ``/10250/0/0``.
  * If ``<data>`` is not present, SLM enters ``slm_data_mode`` and the data is set in the Uplink Data resource (ID: 0) of the App Data Container object (ID: 10250).
    The URI path of the resource instance is indicated as ``/10250/0/0``.

* ``AT#XCARRIER="battery_level",<battery_level>``

  Put the value in ``<battery_level>`` into the Battery Level resource (ID: 9) of the Device object (ID :3).
  ``<battery_level>`` must be an integer value between ``0`` and ``100``.

* ``AT#XCARRIER="battery_status",<battery_status>``

  Set the Battery Status resource (ID: 20) of the Device object (ID: 3).
  ``<battery_status>`` must be an integer value as defined in the OMA LwM2M specification.

* ``AT#XCARRIER="current",<power_source>,<current>``

  Put the value in ``<current>`` into the Power Source Current resource (ID: 8) instance corresponding to one of the Available Power Sources resource (ID: 6) instances of the Device object (ID: 3).
  Refer to the ``AT#XCARRIER="power_sources"`` command for information regarding the supported power sources.
  ``<current>`` must be an integer value specified in milliamperes (mA).

* ``AT#XCARRIER="dereg"``

  Request to send a Deregister request to the server.

* ``AT#XCARRIER="error","add|remove",<error>``

  Update the Error Code resource (ID: 11) of the Device Object (ID: 3) by adding or removing an individual error.
  ``<error>`` must be an integer value as defined in the OMA LwM2M specification.

  * ``AT#XCARRIER="error","add",<error>`` adds a resource instance with the indicated error if one with that error is not present already.
  * ``AT#XCARRIER="error","remove",<error>`` removes the resource instance with the indicated error if it exists.

* ``AT#XCARRIER="link_down"``

  Request to set the modem to flight mode.

* ``AT#XCARRIER="link_up"``

  Request to set the modem to full functionality.

* ``AT#XCARRIER="log_data",<data>``

  Put the value in ``<data>`` into the LogData resource (ID: 4014) of the default EventLog object (ID: 20) instance.
  ``<data>`` must be a hexadecimal string in double quotes.

* ``AT#XCARRIER="memory_free","read|write"[,<memory>]``

  Read or write the Memory Free resource (ID: 10) of the Device object (ID: 3).

  * ``AT#XCARRIER="memory_free","read"`` returns the current value expressed in kilobytes.
  * ``AT#XCARRIER="memory_free","write",<memory>`` puts the value in ``<memory>`` into the resource.
    ``<memory>`` must be an integer value specified in kilobytes.

* ``AT#XCARRIER="memory_total",<memory>``

  Put the value in ``<memory>`` into the Memory Total resource (ID: 21) of the Device object (ID: 3).
  ``<memory>`` must be an integer value specified in kilobytes.

* ``AT#XCARRIER="portfolio","create|read|write",<obj_inst_id>[,<res_inst_id>[,<identity>]]``

  Create an instance of the Portfolio object (ID: 16), or read or write into the Identity (ID: 0) resource of the Portfolio object (ID: 16).

  * ``AT#XCARRIER="portfolio","create",<obj_inst_id>`` creates an instance of the object, where the URI path is specified as ``/16/<obj_inst_id>``.
  * ``AT#XCARRIER="portfolio","read",<obj_inst_id>,<res_inst_id>`` returns the current value of the indicated resource instance, where the URI path is specified as ``/16/<obj_inst_id>/0/<res_inst_id>``.
  * ``AT#XCARRIER="portfolio","write",<obj_inst_id>,<res_inst_id>,<identity>`` puts the value in ``<identity>`` into the indicated resource instance, where the URI path is specified as ``/16/<obj_inst_id>/0/<res_inst_id>``.
    ``<identity>`` must be a string in double quotes.

* ``AT#XCARRIER="power_sources"[,<source1>[,<source2>[,...[,<source8>]]]]``

  Set one or more sources specified in ``<source>`` parameters into the Available Power Sources resource (ID: 6) of the Device object (ID: 3).
  Each ``<source>`` parameter must be an integer value as defined in the OMA LwM2M specification.

* ``AT#XCARRIER="position",<latitude>,<longitude>,<altitude>,<timestamp>,<uncertainty>``

  Put location telemetry values into the corresponding resources of the Location object (ID: 6).

  * ``<latitude>`` specified in the decimal notation of latitude (WGS1984) is put into the Latitude resource (ID: 0).
    Must be a double type value in double quotes.
  * ``<longitude>`` specified in the decimal notation of latitude (WGS1984) is put into the Longitude resource (ID: 1).
    Must be a double type value in double quotes.
  * ``<altitude>`` specified in meters is put into the Altitude resource (ID: 2).
    Must be a float type value in double quotes.
  * ``<timestamp>`` is put into the Timestamp resource (ID: 5).
    Must be an integer value specified in UNIX time.
  * ``<uncertainty>`` specified in meters is put into the Radius resource (ID: 3).
    Must be a float type value in double quotes.

* ``AT#XCARRIER="reboot"``

  Request to reboot the device.
  This allows the library to perform any necessary cleanup before the application resets the device.

* ``AT#XCARRIER="regup"``

  Request to send a Register request (or Registration Update, as dictated by the lifetime) to the server.

* ``AT#XCARRIER="send",<obj_id>,<obj_inst_id>,<res_id>[,<res_inst_id>]``

  Perform a Send operation to send the currently stored data in the indicated resource or resource instance to the server.
  This operation is currently only supported for readable opaque resources.
  The URI path of the resource or resource instance is indicated as ``/<obj_id>/<obj_inst_id>/<res_id>/<res_inst_id>``.

* ``AT#XCARRIER="time"``

  Read the time reported by the device, including the UTC time, the UTC offset and the timezone.
  See examples for response syntax.

* ``AT#XCARRIER="timezone","read|write"[,<timezone>]``

  Read or write the value reported in the Timezone (ID: 15) resource of the Device object (ID: 3) in IANA Timezone (TZ) database format.

  * ``AT#XCARRIER="timezone","read"`` returns the timezone currently stored by the device.
  * ``AT#XCARRIER="timezone","write",<timezone>`` puts the value in ``<timezone>`` into the resource.
    ``<timezone>`` must be a string in double quotes.

* ``AT#XCARRIER="utc_offset","read|write"[,<utc_offset>]``

  Read or write the UTC Offset resource (ID: 14) of the Device object (ID: 3).

  * ``AT#XCARRIER="utc_offset","read"`` returns the UTC offset currently in effect for the device as per ISO 8601.
  * ``AT#XCARRIER="utc_offset","write",<utc_offset>`` puts the value in ``<utc_offset>`` into the resource.
    ``<utc_offset>`` must be an integer value specified in minutes.

* ``AT#XCARRIER="utc_time","read|write"[,<utc_time>]``

  Read or write the Current Time resource (ID: 13) of the Device object (ID: 3).

  * ``AT#XCARRIER="utc_time","read"`` returns the current UNIX time of the device.
  * ``AT#XCARRIER="utc_time","write",<utc_time>`` puts the value in ``<utc_time>`` into the resource.
    ``<utc_time>`` must be an integer value specified in UNIX time.

* ``AT#XCARRIER="velocity",<heading>,<speed_h>,<speed_v>,<uncertainty_h>,<uncertainty_v>``

  Set or update the latest velocity information that will be mapped into the Velocity resource (ID: 4) and Speed resource (ID: 6) of the Location object (ID: 6).

  * ``<heading>`` is the horizontal direction of movement in degrees clockwise from North.
    Must be an integer value between ``0`` and ``359``.
  * ``<speed_h>`` is the horizontal non-negative speed in meters per second.
    Must be a float type value in double quotes.
  * ``<speed_v>`` is the vertical speed in meters per second.
    A positive value indicates upward motion, while a negative value indicates downward motion.
    Must be a float type value in double quotes.
  * ``<uncertainty_h>`` is the horizontal uncertainty speed in meters per second.
    Must be a non-negative float type value in double quotes.
  * ``<uncertainty_v>`` is the vertical uncertainty speed in meters per second.
    Must be a non-negative float type value in double quotes.

* ``AT#XCARRIER="voltage",<power_source>,<voltage>``

  Put the value in ``<voltage>`` into the Power Source Voltage resource (ID: 7) instance corresponding to one of the Available Power Sources resource (ID: 6) instances of the Device object (ID: 3).
  Refer to the ``AT#XCARRIER="power_sources"`` command for information regarding the supported power sources.
  ``<voltage>`` must be an integer value specified in millivolts.

Response syntax
~~~~~~~~~~~~~~~

The response syntax depends on the commands used.

Examples
~~~~~~~~

::

   AT#XCARRIER="time","read"
   #XCARRIER: UTC_TIME: 2022-12-30T14:56:46Z, UTC_OFFSET: 60, TIMEZONE: Europe/Paris
   OK

::

   AT#XCARRIER="error","add",5
   OK

   AT#XCARRIER="error","remove",5
   OK

::

   AT#XCARRIER="power_sources",1,2,6
   OK

::

   AT#XCARRIER="portfolio","read",2,3
   #XCARRIER: LwM2M carrier 3.1.0
   OK

::

   AT#XCARRIER="reboot"
   OK

::

   AT#XCARRIER="position","63.43","10.47","48",1708684683,"30.5"
   OK

::

   AT#XCARRIER="send",19,0,0,0
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

LwM2M Carrier library configuration #XCARRIERCFG
================================================

The ``#XCARRIERCFG`` command allows you to configure the LwM2M carrier library.
The settings are stored and applied persistently.

.. note::
   To use ``#XCARRIERCFG``, the :kconfig:option:`CONFIG_LWM2M_CARRIER_SETTINGS` Kconfig option must be enabled.
   For more details on LwM2M carrier library configuration, see the :ref:`lwm2m_configuration` section of the library's documentation.

Set command
-----------

The set command allows you to configure the LwM2M carrier library.

Syntax
~~~~~~

::

   AT#XCARRIERCFG=<cmd>[,<param1>[,<param2>]..]

The ``<cmd>`` command is a string, and can be used as follows:

* ``AT#XCARRIERCFG="apn"[,<apn>]``

  Configure the LwM2M carrier library to use a custom APN specified in ``<apn>`` when connecting to the device management network.
  ``<apn>`` must be a string in double quotes.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN` Kconfig option.

* ``AT#XCARRIERCFG="auto_register"[,<0|1>]``

  Set a flag to automatically register to the device management server when attaching to the network.
  When this configuration is disabled, the user must trigger the registration manually through the ``AT#XCARRIER="regup"`` command after recovering network coverage.
  This command accepts two possible input parameters: ``0`` to disable or ``1`` to enable.

* ``AT#XCARRIERCFG="auto_startup"[,<0|1>]``

  Set a flag to automatically apply the enabled settings to the LwM2M carrier library configuration and connect to the device management network.
  This command accepts two possible input parameters: ``0`` to disable or ``1`` to enable.
  This command is not available when the :kconfig:option:`CONFIG_SLM_CARRIER_AUTO_STARTUP` Kconfig option is enabled.

* ``AT#XCARRIERCFG="carriers"[,"all"|<carrier1>[,<carrier2>[,...[,<carrier6>]]]]``

  Configure the networks in which the LwM2M carrier library will apply (see the :ref:`general_options_lwm2m` section of the library's documentation).
  By default, any network is allowed. The input parameters are mapped as follows:

  * ``all`` - Any network allowed.
  * ``0`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC`.
  * ``1`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_VERIZON`.
  * ``2`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_ATT`.
  * ``3`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS`.
  * ``4`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_T_MOBILE`.
  * ``5`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_SOFTBANK`.

* ``AT#XCARRIERCFG="coap_con_interval"[,<interval>]``

  Configure how often the LwM2M carrier library is to send the Notify operation as a CoAP Confirmable message instead of a Non-Confirmable message.
  ``<interval>`` must be an integer value specified in seconds.
  Two special values may also be used: ``-1`` to always use Confirmable notifications, or ``0`` to use the default interval of 86400 seconds.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_COAP_CON_INTERVAL` Kconfig option.

* ``AT#XCARRIERCFG="download_timeout"[,<timeout>]``

  Configure the time allowed for a single firmware image download before it is aborted.
  This configuration is only supported for Push delivery method of firmware images.
  ``<timeout>`` must be an integer value specified in minutes.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_FIRMWARE_DOWNLOAD_TIMEOUT` Kconfig option.

* ``AT#XCARRIERCFG="config_enable"[,<0|1>]``

  Set flag to apply the stored settings to the general Kconfig options (see the :ref:`general_options_lwm2m` section of the library's documentation).

* ``AT#XCARRIERCFG="device_enable"[,<0|1>]``

  Set flag to apply the stored settings to the device Kconfig options (see the :ref:`device_options_lwm2m` section of the library's documentation).

* ``AT#XCARRIERCFG="device_type"[,<device_type>]``

  Configure the value in ``<device_type>`` to be put into the Device Type resource (ID: 17) of the Device object (ID: 3).
  ``<device_type>`` must be a string in double quotes.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_TYPE` Kconfig option.

* ``AT#XCARRIERCFG="hardware_version"[,<version>]``

  Configure the value in ``<version>`` to be put into the Hardware Version resource (ID: 18) of the Device object (ID: 3).
  ``<version>`` must be a string in double quotes.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_HARDWARE_VERSION` Kconfig option.

* ``AT#XCARRIERCFG="manufacturer"[,<manufacturer>]``

  Configure the value in ``<manufacturer>`` to be put into the Manufacturer resource (ID: 0) of the Device object (ID: 3).
  ``<manufacturer>`` must be a string in double quotes.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_MANUFACTURER` Kconfig option.

* ``AT#XCARRIERCFG="model_number"[,<model_number>]``

  Configure the value in ``<model_number>`` to be put into the Model Number resource (ID: 1) of the Device object (ID: 3).
  ``<model_number>`` must be a string in double quotes.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_MODEL_NUMBER` Kconfig option.

* ``AT#XCARRIERCFG="software_version"[,<version>]``

  Configure the value in ``<version>`` to be put into the Software Version resource (ID: 19) of the Device object (ID: 3).
  ``<version>`` must be a string in double quotes.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_SOFTWARE_VERSION` Kconfig option.

* ``AT#XCARRIERCFG="device_serial_no_type"[,<device_serial_no_type>]``

  Configure the Device Serial Number type to be used in LG U+ network.
  The ``<device_serial_no_type>`` must be an integer value.
  It accepts the following values:

  * ``0`` - :kconfig:option:`LWM2M_CARRIER_LG_UPLUS_IMEI`.
  * ``1`` - :kconfig:option:`LWM2M_CARRIER_LG_UPLUS_2DID`.

  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NUMBER` Kconfig option.

* ``AT#XCARRIERCFG="service_code"[,<service_code>]``

  Configure the Service Code registered for this device with LG U+.
  ``<service_code>`` must be a string in double quotes.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE` Kconfig option.

* ``AT#XCARRIERCFG="pdn_type"[,<pdn_type>]``

  Configure the PDN type of the custom APN configured through the ``AT#XCARRIERCFG="apn"`` command.
  ``<pdn_type>`` must be an integer value.
  It accepts the following values:

  * ``0`` - :c:macro:`LWM2M_CARRIER_PDN_TYPE_IPV4V6`.
  * ``1`` - :c:macro:`LWM2M_CARRIER_PDN_TYPE_IPV4`.
  * ``2`` - :c:macro:`LWM2M_CARRIER_PDN_TYPE_IPV6`.
  * ``3`` - :c:macro:`LWM2M_CARRIER_PDN_TYPE_NONIP`.

  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_PDN_TYPE` Kconfig option.

* ``AT#XCARRIERCFG="queue_mode"[,<0|1>]``

  Configure whether the LwM2M carrier library is to inform the server that it may be disconnected for an extended period of time.
  This configuration corresponds to the Queue Mode Operation as defined in the OMA LwM2M specification.
  This command accepts two possible input parameters: ``0`` to disable or ``1`` to enable.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_QUEUE_MODE` Kconfig option.

* ``AT#XCARRIERCFG="binding"[,<binding>]``

  Configure the binding over which the LwM2M carrier library is to connect to the device management network.
  ``<binding>`` must be a string in double quotes. It accepts any combination of the following values:

  * ``"U"`` - :kconfig:option:`LWM2M_CARRIER_SERVER_BINDING_U`.
  * ``"N"`` - :kconfig:option:`LWM2M_CARRIER_SERVER_BINDING_N`.

  Additionally, an empty ``<binding>`` resets the configuration to default setting (factory configuration).
  For details, see the :kconfig:option:`CONFIG_LWM2M_SERVER_BINDING_CHOICE` Kconfig option.

* ``AT#XCARRIERCFG="is_bootstrap"[,<0|1>]``

  Indicate whether the custom server configured through the ``AT#XCARRIERCFG="uri"`` command is a bootstrap server.
  This command accepts two possible input parameters: ``0`` if it is not a bootstrap server or ``1`` if it is a bootstrap server.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_BOOTSTRAP_SERVER` Kconfig option.

* ``AT#XCARRIERCFG="lifetime"[,<lifetime>]``

  Configure the lifetime of the custom server configured through the ``AT#XCARRIERCFG="uri"`` command.
  ``<lifetime>`` must an integer value specified in seconds.
  This configuration is ignored if the custom server is a bootstrap server.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_LIFETIME` Kconfig option.

* ``AT#XCARRIERCFG="sec_tag"[,<sec_tag>]``

  Configure the security tag that stores the credentials to be used to set up the DTLS session with the custom server configured through the ``AT#XCARRIERCFG="uri"`` command.
  ``<sec_tag>`` must be an integer value specifying the security tag number to be used.
  This configuration is ignored for non-secure connections.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` Kconfig option.

* ``AT#XCARRIERCFG="uri"[,<uri>]``

  Configure the URI of a custom server that the library is to connect to.
  ``<uri>`` must be a string in double quotes.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` Kconfig option.

* ``AT#XCARRIERCFG="session_idle_timeout"[,<session_idle_timeout>]``

  Configure how long a DTLS session used by the library can be idle before it is closed.
  ``<session_idle_timeout>`` must be an integer value specified in seconds.
  Two special values may also be used: ``-1`` to disable the session idle timeout, or ``0`` to use the default interval of 60 seconds.
  For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT` Kconfig option.


If a valid command string is used without any parameter, the current value of the corresponding configuration will be returned in the response, as in the examples shown below.

Response syntax
~~~~~~~~~~~~~~~

The response syntax depends on the commands used.

Examples
~~~~~~~~

::

   AT#XCARRIERCFG="auto_startup"
   #XCARRIERCFG: 0
   OK

   AT#XCARRIERCFG="auto_startup",1
   OK

   AT#XCARRIERCFG="auto_startup"
   #XCARRIERCFG: 1
   OK

::

   AT#XCARRIERCFG="manufacturer","Nordic Semiconductor ASA"
   OK

   AT#XCARRIERCFG="manufacturer"
   #XCARRIERCFG: Nordic Semiconductor ASA
   OK

::

   AT#XCARRIERCFG="apn","custom.APN"
   OK

   AT#XCARRIERCFG="apn"
   #XCARRIERCFG: custom.APN
   OK

::

   AT#XCARRIERCFG="carriers","all"
   OK

   AT#XCARRIERCFG="carriers"
   #XCARRIERCFG: all

   AT#XCARRIERCFG="carriers",1,3
   OK

   AT#XCARRIERCFG="carriers"
   #XCARRIERCFG: 1, 3
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
