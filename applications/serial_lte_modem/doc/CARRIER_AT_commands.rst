.. _SLM_AT_CARRIER:

LwM2M carrier library AT commands
*********************************

.. contents::
   :local:
   :depth: 2

The following commands list contains AT commands related to the LwM2M carrier library.

Carrier event #XCARRIEREVT
==========================

The ``#XCARRIEREVT`` is an unsolicited notification that indicates the event of the LwM2M carrier library.

Unsolicited notification
------------------------

It indicates the event of the LwM2M carrier library.

Syntax
~~~~~~

::

   #XCARRIEREVT: <evt_type>,<info>[,<path>]
   <data>

* The ``<evt_type>`` value is an integer indicating the type of the event.
  It can return the following values:

  * ``1`` - Request to set the modem to full functional mode.
  * ``2`` - Request to set the modem to flight functional mode.
  * ``3`` - Request to set the modem to minimum functional mode.
  * ``4`` - Bootstrap sequence complete.
  * ``5`` - Device registered successfully to the device management servers.
  * ``6`` - Connection to the server failed.
  * ``7`` - Firmware update started.
  * ``8`` - Firmware updated successfully.
  * ``9`` - Request to perform an application reboot.
  * ``10`` - Modem domain event received.
  * ``11`` - Data received through the App Data Container object.
  * ``12`` - Request to initialize the modem.
  * ``13`` - Request to shut down the modem.
  * ``20`` - LwM2M carrier library error occurred.

* The ``<info>`` value is an integer providing additional information about the event.
  It can return the following values:

  * ``0`` - Success or nothing to report.
  * *Negative value* - Failure or request to defer an application reboot or modem functional mode change.
  * *Positive value* - Number of bytes received through the App Data Container object or the Binary App Data Container object.

* The ``<path>`` value is a string only present in an event of type ``11``.
  It describes the URI path of the resource or the resource instance that received the data.

* The ``<data>`` parameter is a string that contains the data received through the App Data Container object or the Binary App Data container object.

The events of type ``1``, ``2`` and ``3`` will typically be followed by an error event of type ``20``.
This indicates to the application that the library is waiting for the appropriate modem functional mode change.

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

* ``AT#XCARRIER="app_data"[,<data>][,<instance_id>,<resource_instance_id>]``
* ``AT#XCARRIER="battery_level",<battery_level>``
* ``AT#XCARRIER="battery_status",<battery_status>``
* ``AT#XCARRIER="current",<power_source>,<current>``
* ``AT#XCARRIER="error","add|remove",<error>``
* ``AT#XCARRIER="link_down"``
* ``AT#XCARRIER="link_up"``
* ``AT#XCARRIER="log_data",<data>``
* ``AT#XCARRIER="memory_free","read|write"[,<memory>]``
* ``AT#XCARRIER="memory_total",<memory>``
* ``AT#XCARRIER="portfolio","create|read|write",<instance_id>[,<identity_type>[,<identity>]]``
* ``AT#XCARRIER="power_sources"[,<source1>[,<source2>[,...[,<source8>]]]]``
* ``AT#XCARRIER="position",<latitude>,<longitude>,<altitude>,<timestamp>,<uncertainty>``
* ``AT#XCARRIER="reboot"``
* ``AT#XCARRIER="time"``
* ``AT#XCARRIER="timezone","read|write"[,<timezone>]``
* ``AT#XCARRIER="utc_offset","read|write"[,<utc_offset>]``
* ``AT#XCARRIER="utc_time","read|write"[,<utc_time>]``
* ``AT#XCARRIER="velocity",<heading>,<speed_h>,<speed_v>,<uncertainty_h>,<uncertainty_v>``
* ``AT#XCARRIER="voltage",<power_source>,<voltage>``

The values of the parameters depend on the command string used.
Specifying ``instance_id`` and ``resource_instance_id`` in the ``app_data`` command sends the data to the Binary App Data Container object instead of the App Data Container object.
When using the ``app_data`` command, if no attributes are specified, SLM enters ``slm_data_mode``.
``slm_data_mode`` is only supported when using the App Data Container object.

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
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN` Kconfig option.
* ``AT#XCARRIERCFG="auto_startup"[,<0|1>]``
  * Set flag to automatically apply the enabled settings to the library configuration and connect to the device management server.
* ``AT#XCARRIERCFG="bootstrap_smartcard"[,<0|1>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_BOOTSTRAP_SMARTCARD` Kconfig option.
* ``AT#XCARRIERCFG="carriers"[,"all"|<carrier1>[<carrier2>[,...[,<carrier6>]]]]``
  * Choose the networks in which the LwM2M carrier library will apply (see the :ref:`general_options_lwm2m` section of the library's documentation).
* ``AT#XCARRIERCFG="coap_con_interval"[,<interval>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_COAP_CON_INTERVAL` Kconfig option.
* ``AT#XCARRIERCFG="config_enable"[,<0|1>]``
  * Set flag to apply the stored settings to the general Kconfig options (see the :ref:`general_options_lwm2m` section of the library's documentation).
* ``AT#XCARRIERCFG="device_enable"[,<0|1>]``
  * Set flag to apply the stored settings to the device Kconfig options (see the :ref:`device_options_lwm2m` section of the library's documentation).
* ``AT#XCARRIERCFG="device_type"[,<device_type>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_TYPE` Kconfig option.
* ``AT#XCARRIERCFG="hardware_version"[,<version>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_HARDWARE_VERSION` Kconfig option.
* ``AT#XCARRIERCFG="manufacturer"[,<manufacturer>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_MANUFACTURER` Kconfig option.
* ``AT#XCARRIERCFG="model_number"[,<model_number>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_MODEL_NUMBER` Kconfig option.
* ``AT#XCARRIERCFG="software_version"[,<version>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_DEVICE_SOFTWARE_VERSION` Kconfig option.
* ``AT#XCARRIERCFG="device_serial_no_type"[,<device_serial_no_type>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NUMBER` Kconfig option.
* ``AT#XCARRIERCFG="service_code"[,<service_code>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE` Kconfig option.
* ``AT#XCARRIERCFG="pdn_type"[,<pdn_type>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_PDN_TYPE` Kconfig option.
* ``AT#XCARRIERCFG="binding"[,<binding>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_SERVER_BINDING_CHOICE` Kconfig option.
* ``AT#XCARRIERCFG="server_enable"[,<0|1>]``
  * Set flag to apply the stored settings to the server Kconfig options (see the :ref:`server_options_lwm2m` section of the library's documentation).
* ``AT#XCARRIERCFG="is_bootstrap"[,<0|1>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_BOOTSTRAP_SERVER` Kconfig option.
* ``AT#XCARRIERCFG="lifetime"[,<lifetime>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_LIFETIME` Kconfig option.
* ``AT#XCARRIERCFG="sec_tag"[,<sec_tag>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` Kconfig option.
* ``AT#XCARRIERCFG="uri"[,<uri>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` Kconfig option.
* ``AT#XCARRIERCFG="session_idle_timeout"[,<session_idle_timeout>]``
  * For details, see the :kconfig:option:`CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT` Kconfig option.

The values of the parameters depend on the command string used.
If a valid command string is used without any parameter, the current value of the corresponding configuration will be returned in the response, as in the examples shown below.
Boolean configurations, such as ``auto_startup`` or ``is_bootstrap``, only accept ``0`` (disable) and ``1`` (enable) as input parameters.

The ``carriers`` configuration allows to choose the networks in which the LwM2M carrier library will apply.
Possible inputs are mapped as follows:

* ``all`` - Any network allowed.
* ``0`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_GENERIC`.
* ``1`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_VERIZON`.
* ``2`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_ATT`.
* ``3`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS`.
* ``4`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_T_MOBILE`.
* ``5`` - :kconfig:option:`CONFIG_LWM2M_CARRIER_SOFTBANK`.

The ``device_serial_no_type`` configuration accepts the following values:

* ``0`` - :kconfig:option:`LWM2M_CARRIER_LG_UPLUS_IMEI`.
* ``1`` - :kconfig:option:`LWM2M_CARRIER_LG_UPLUS_2DID`.

The ``pdn_type`` configuration accepts the following values:

* ``0`` - :c:macro:`LWM2M_CARRIER_PDN_TYPE_IPV4V6`.
* ``1`` - :c:macro:`LWM2M_CARRIER_PDN_TYPE_IPV4`.
* ``2`` - :c:macro:`LWM2M_CARRIER_PDN_TYPE_IPV6`.
* ``3`` - :c:macro:`LWM2M_CARRIER_PDN_TYPE_NONIP`.

The ``binding`` configuration accepts the following values:

* ``"U"`` - :kconfig:option:`LWM2M_CARRIER_SERVER_BINDING_U`.
* ``"N"`` - :kconfig:option:`LWM2M_CARRIER_SERVER_BINDING_N`.

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
