.. _SLM_AT_gen:

Generic AT commands
*******************

.. contents::
   :local:
   :depth: 2

The following commands list contains generic AT commands.

SLM version #XSLMVER
====================

The ``#XSLMVER`` command return the versions of the |NCS| in which the SLM application is built.
It also returns the version of the modem library that SLM uses to communicate with the modem.

Set command
-----------

The set command returns the versions of the |NCS| and the modem library.

Syntax
~~~~~~

::

   #XSLMVER

Response syntax
~~~~~~~~~~~~~~~

::

   #XSLMVER: <ncs_version>,<libmodem_version>

The ``<ncs_version>`` value returns a string containing the version of the |NCS|.

The ``<libmodem_version>`` value returns a string containing the version of the modem library.

Example
~~~~~~~

The following command example reads the versions:

::

   AT#XSLMVER
   #XSLMVER: "2.3.0","2.3.0"
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

SLM proprietary command list #XCLAC
===================================

The ``#XCLAC`` command requests the list of the proprietary SLM commands.

Set command
-----------

The set command requests the list of the proprietary SLM commands.
It is an add-on for ``AT+CLAC``, which lists all modem AT commands.

Syntax
~~~~~~

::

   #XCLAC

Response syntax
~~~~~~~~~~~~~~~

::

   <command list>

The ``<command list>`` value returns a list of values representing all the ``#X*`` commands followed by <CR><LF>.

Example
~~~~~~~

::

   AT#XCLAC
   AT#XSLMVER
   AT#XSLEEP
   AT#XCLAC
   AT#XSOCKET
   AT#XBIND
   AT#XCONNECT
   AT#XSEND
   AT#XRECV
   AT#XSENDTO
   AT#XRECVFROM
   AT#XPING
   AT#XGPS
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Power saving #XSLEEP
====================

The ``#XSLEEP`` command makes the nRF9160 System in Package (SiP) enter idle or sleep mode.

If you want to do power measurements on the nRF9160 development kit while running the SLM application, disable unused peripherals.

Set command
-----------

The set command makes the nRF9160 SiP enter either Idle or Sleep mode.

Syntax
~~~~~~

::

   #XSLEEP=<sleep_mode>

The ``<sleep_mode>`` parameter accepts only the following integer values:

* ``0`` - Deprecated.
* ``1`` - Enter Sleep.
  In this mode, both the SLM service and the LTE connection are terminated.

  The nRF9160 SiP can be woken up using the :ref:`CONFIG_SLM_WAKEUP_PIN <CONFIG_SLM_WAKEUP_PIN>`.

* ``2`` - Enter Idle.

  In this mode, both the SLM service and the LTE connection are maintained.
  The nRF9160 SiP can be made to exit idle using the :ref:`CONFIG_SLM_WAKEUP_PIN <CONFIG_SLM_WAKEUP_PIN>`.
  If the :ref:`CONFIG_SLM_INDICATE_PIN <CONFIG_SLM_INDICATE_PIN>` is defined, SLM toggle this GPIO when there is data for MCU.
  MCU could in turn make SLM to exit idle by :ref:`CONFIG_SLM_WAKEUP_PIN <CONFIG_SLM_WAKEUP_PIN>`.
  The data is buffered during the idle status and sent to MCU after exiting the idle status.

.. note::

   * This parameter does not accept ``0`` anymore.
   * If the modem is on, entering Sleep mode sends a ``+CFUN=0`` command to the modem, which causes a non-volatile memory (NVM) write.
     Take the NVM wear into account, or put the modem in flight mode by issuing a ``AT+CFUN=4`` before Sleep mode.

Examples
~~~~~~~~

::

   AT#XSLEEP=0
   ERROR

::

   AT#XSLEEP=1
   OK

See the following for an example of when the modem is on:

::

   AT+CFUN=4
   OK

   AT#XSLEEP=1
   OK

::

   AT#XSLEEP=2
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command tests the existence of the AT command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XSLEEP=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSLEEP: <list of shutdown_mode>

Example
~~~~~~~

::

   #XSLEEP: (1,2)
   OK

Power off #XSHUTDOWN
====================

The ``#XSHUTDOWN`` command makes the nRF9160 SiP enter System OFF mode, which is the deepest power saving mode.

Set command
-----------

The set command makes the nRF9160 SiP enter System OFF mode.

Syntax
~~~~~~

::

   #XSHUTDOWN

.. note::

   In this case the nRF9160 SiP cannot be woken up using the :ref:`CONFIG_SLM_WAKEUP_PIN <CONFIG_SLM_WAKEUP_PIN>`..

Example
~~~~~~~~

::

   AT#XSHUTDOWN
   OK


Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Reset #XRESET
=============

The ``#XRESET`` command performs a soft reset of the nRF9160 SiP.

Set command
-----------

The set command resets the nRF9160 SiP.

Syntax
~~~~~~

::

   #XRESET

Example
~~~~~~~~

::

   AT#XRESET
   OK
   Ready

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Modem reset #XMODEMRESET
========================

The ``#XMODEMRESET`` command performs a reset of the modem.

The modem is set to minimal function mode (via ``+CFUN=0``) before being reset.
The SLM application is not restarted.
After the command returns, the modem will be in minimal function mode.

Set command
-----------

The set command resets the modem.

Syntax
~~~~~~

::

   #XMODEMRESET

Response syntax
~~~~~~~~~~~~~~~

::

   #XMODEMRESET: <result>[,<error_code>]

* The ``<result>`` parameter is an integer indicating the result of the command.
  It can have the following values:

  * ``0`` - Success.
  * *Positive value* - On failure, indicates the step that failed.

* The ``<error_code>`` parameter is an integer.
  It is only printed when the modem reset was not successful and is the error code indicating the reason for the failure.

Example
~~~~~~~~

::

   AT#XMODEMRESET

   #XMODEMRESET: 0

   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

SLM UART #XSLMUART
==================

The ``#XSLMUART`` command manages the UART settings.

Set command
-----------

The set command changes the UART baud rate setting.
This setting is stored in the flash memory and applied during the application startup.

Syntax
~~~~~~
::

   #XSLMUART[=<baud_rate>]

The ``<baud_rate>`` parameter is an integer.
It accepts the following values:

* ``1200`` - 1200 bps
* ``2400`` - 2400 bps
* ``4800`` - 4800 bps
* ``9600`` - 9600 bps
* ``14400`` - 14400 bps
* ``19200`` - 19200 bps
* ``38400`` - 38400 bps
* ``57600`` - 57600 bps
* ``115200`` - 115200 bps
* ``230400`` - 230400 bps
* ``460800`` - 460800 bps
* ``921600`` - 921600 bps
* ``1000000`` - 1000000 bps

Its default value is ``115200``.
When not specified, it is set to the last value set for the variable and stored in the flash memory.
If there is no value stored for the variable, it is set to its default value.
If not specified, the previous value is used.

Response syntax
~~~~~~~~~~~~~~~

There is no response.

Example
~~~~~~~

::

   AT#XSLMUART=1000000
   OK

Read command
------------

The read command shows the current UART settings.

Syntax
~~~~~~

::

   #XSLMUART?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSLMUART: <baud_rate>

Example
~~~~~~~

::

   AT#XSLMUART?
   #XSLMUART: 115200
   OK

Test command
------------

The test command tests the existence of the AT command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XSLMUART=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSLMUART: (list of the available baud rate options)

Example
~~~~~~~

::

   AT#XSLMUART=?
   #XSLMUART: (1200,2400,4800,9600,14400,19200,38400,57600,115200,230400,460800,921600,1000000)

Device UUID #XUUID
==================

The ``#XUUID`` command requests the device UUID.

Set command
-----------

The set command returns the device UUID.

Syntax
~~~~~~

::

   #XUUID

Response syntax
~~~~~~~~~~~~~~~

::

   #XUUID: <device-uuid>

The ``<device-uuid>`` value returns a string indicating the UUID of the device.

Example
~~~~~~~

::

  AT#XUUID

  #XUUID: 50503041-3633-4261-803d-1e2b8f70111a

  OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Native TLS CMNG #XCMNG
======================

The ``#XCMNG`` command manages the credentials to support :ref:`CONFIG_SLM_NATIVE_TLS <CONFIG_SLM_NATIVE_TLS>`.
This command is similar to the modem ``%CMNG`` command.

Set command
-----------

The set command is used for credential storage management.
The command writes, reads, deletes, and checks the existence of keys and certificates.

Syntax
~~~~~~

The following is the syntax when :ref:`CONFIG_SLM_NATIVE_TLS <CONFIG_SLM_NATIVE_TLS>` is selected:
::

   #XCMNG=<opcode>[,<sec_tag>[,<type>[,<content>]]]

The ``<opcode>`` parameter is an integer.
It accepts the following values:

* ``0`` - Write a credential.
* ``1`` - List credentials (currently not supported).
* ``2`` - Read a credential (currently not supported).
* ``3`` - Delete a credential.

The ``<sec_tag>`` parameter is an integer ranging between ``0`` and ``2147483647``.
It is mandatory for *write*, *read*, and *delete* operations.
It is optional for *list* operations.

The ``<type>`` parameter is an integer.
It accepts the following values:

* ``0`` - Root CA certificate (ASCII text)
* ``1`` - Certificate (ASCII text)
* ``2`` - Private key (ASCII text)

The ``<content>`` parameter is a string.
It is mandatory if ``<opcode>`` is ``0`` (write a credential).
It is the content of a Privacy Enhanced Mail (PEM) file enclosed in double quotes (X.509 PEM entities).
An empty string is not allowed.

Response syntax
~~~~~~~~~~~~~~~

There is no response.

Example
~~~~~~~

::

   AT#XCMNG=0,10,0,"-----BEGIN CERTIFICATE-----
   MIICpTCCAkugAwIBAgIUS+wVM0VsVmpDIV8NTW8N2KEdRdowCgYIKoZIzj0EAwIw
   gacxCzAJBgNVBAYTAlRXMQ8wDQYDVQQIDAZUYWl3YW4xDzANBgNVBAcMBlRhaXBl
   aTEWMBQGA1UECgwNTm9yZGljIFRhaXBlaTEOMAwGA1UECwwFU2FsZXMxETAPBgNV
   BAMMCExhcnJ5IENBMTswOQYJKoZIhvcNAQkBFixsYXJyeS52ZXJ5bG9uZ2xvbmds
   b25nbG9uZ2xvbmdAbm9yZGljc2VtaS5ubzAeFw0yMDExMTcxMTE3MDlaFw0zMDEx
   MTUxMTE3MDlaMIGnMQswCQYDVQQGEwJUVzEPMA0GA1UECAwGVGFpd2FuMQ8wDQYD
   VQQHDAZUYWlwZWkxFjAUBgNVBAoMDU5vcmRpYyBUYWlwZWkxDjAMBgNVBAsMBVNh
   bGVzMREwDwYDVQQDDAhMYXJyeSBDQTE7MDkGCSqGSIb3DQEJARYsbGFycnkudmVy
   eWxvbmdsb25nbG9uZ2xvbmdsb25nQG5vcmRpY3NlbWkubm8wWTATBgcqhkjOPQIB
   BggqhkjOPQMBBwNCAASvk+LcLXwteWokU1In+FQUWkkbQhkpW61u7d0jV1y/eF3Q
   PTDAoEz//SnU1kIZccAqV64fFrrd2nkXknLCrhtxo1MwUTAdBgNVHQ4EFgQUMYSO
   cWPI+SQUs1oVatNQvN/F0UowHwYDVR0jBBgwFoAUMYSOcWPI+SQUs1oVatNQvN/F
   0UowDwYDVR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBFAiB2IrzpUmQqcUIw
   OVqOMNAlzR6v4YHlI9InxU01quIRtQIhAOTITnLNuA0r0571SSBKZyrNGzxJxcPO
   FDkGjew9OVov
   -----END CERTIFICATE-----"

   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Modem fault #XMODEM
===================

The application monitors the modem status.
When the application detects a *modem fault*, it sends the ``#XMODEM`` unsolicited notification.

Unsolicited notification
------------------------

The application sends the following unsolicited notification when it detects a modem fault:

::

   #XMODEM: FAULT,<reason>,<program_count>

The ``<reason>`` value returns a hexadecimal integer indicating the reason of the modem fault.
The ``<program_count>`` value returns a hexadecimal integer indicating the address of the modem fault.

The application sends the following unsolicited notification when it shuts down libmodem:

::

   #XMODEM: SHUTDOWN,<result>

The ``<result>`` value returns an integer indicating the result of the shutdown of libmodem.

The application sends the following unsolicited notification when it re-initializes libmodem:

::

   #XMODEM: INIT,<result>

The ``<result>`` value returns an integer indicating the result of the re-initialization of libmodem.

.. note::
   After libmodem is re-initialized, the MCU side must restart the current active service as follows:

   1. Stopping the service.
      For example, disconnecting the TCP connection and closing the socket.
   #. Connecting again using LTE.
   #. Restarting the service.
      For example, opening the socket and re-establishing the TCP connection.
