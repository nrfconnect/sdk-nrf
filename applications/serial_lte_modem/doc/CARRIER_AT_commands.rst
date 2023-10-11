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

   #XCARRIEREVT: <evt_type>,<info>
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

* ``AT#XCARRIER="auto_connect","read|write"[,<auto-connect-flag>]``
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

   AT#XCARRIER="auto_connect","read"
   #XCARRIER: auto_connect 1
   OK

   AT#XCARRIER="auto_connect","write",0
   OK

   AT#XCARRIER="auto_connect","read"
   #XCARRIER: auto_connect 0
   OK

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
