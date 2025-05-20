.. _SLM_AT_gen:

Generic AT commands
*******************

.. contents::
   :local:
   :depth: 2

This page describes generic AT commands.

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

   #XSLMVER: <ncs_version>,<libmodem_version>[,<customer_version>]

The ``<ncs_version>`` value is a string containing the version of the |NCS|.

The ``<libmodem_version>`` value is a string containing the version of the modem library.

The ``<customer_version>`` value is the :ref:`CONFIG_SLM_CUSTOMER_VERSION <CONFIG_SLM_CUSTOMER_VERSION>` string, if defined.

Example
~~~~~~~

The following command example reads the versions:

::

   AT#XSLMVER
   #XSLMVER: "2.5.0","2.5.0-lte-5ccd2d4dd54c"
   OK

   AT#XSLMVER
   #XSLMVER: "2.5.99","2.5.0-lte-5ccd2d4dd54c","Onomondo 2.1.0"
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

The ``#XSLEEP`` command makes the nRF91 Series System in Package (SiP) enter idle or sleep mode.

If you want to do power measurements on the nRF91 Series development kit while running the SLM application, disable unused peripherals.

Set command
-----------

The set command makes the nRF91 Series SiP enter either Idle or Sleep mode.

Syntax
~~~~~~

::

   #XSLEEP=<sleep_mode>

The ``<sleep_mode>`` parameter accepts only the following integer values:

* ``1`` - Enter Sleep.
  In this mode, both the SLM service and the LTE connection are terminated.

  The nRF91 Series SiP can be woken up using the pin specified with the :ref:`CONFIG_SLM_POWER_PIN <CONFIG_SLM_POWER_PIN>` Kconfig option.

* ``2`` - Enter Idle.
  In this mode, both the SLM service and the LTE connection are maintained.

  The nRF91 Series SiP can be made to exit idle using the pin specified with the :ref:`CONFIG_SLM_POWER_PIN <CONFIG_SLM_POWER_PIN>` Kconfig option.
  If the :ref:`CONFIG_SLM_INDICATE_PIN <CONFIG_SLM_INDICATE_PIN>` Kconfig option is defined, SLM toggles the specified pin when there is data for the MCU to read.
  The MCU can in turn make SLM exit idle by toggling the pin specified with the :ref:`CONFIG_SLM_POWER_PIN <CONFIG_SLM_POWER_PIN>` Kconfig option.
  The data is buffered when SLM is idle and sent to the MCU after having exited idle.

.. note::

   * If the modem is on, entering Sleep mode (by issuing ``AT#XSLEEP=1`` ) sends a ``+CFUN=0`` command to the modem, which causes a write to non-volatile memory (NVM).
     Take the NVM wear into account, or put the modem in flight mode by issuing ``AT+CFUN=4`` before Sleep mode.

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

The ``#XSHUTDOWN`` command makes the nRF91 Series SiP enter System OFF mode, which is the deepest power saving mode.

Set command
-----------

The set command makes the nRF91 Series SiP enter System OFF mode.

Syntax
~~~~~~

::

   #XSHUTDOWN

.. note::

   In this case the nRF91 Series SiP cannot be woken up using the pin specified with the :ref:`CONFIG_SLM_POWER_PIN <CONFIG_SLM_POWER_PIN>` Kconfig option.

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

The ``#XRESET`` command performs a soft reset of the nRF91 Series SiP.

Set command
-----------

The set command resets the nRF91 Series SiP.

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

The ``#XCMNG`` command manages the credentials to support :ref:`CONFIG_SLM_NATIVE_TLS <CONFIG_SLM_NATIVE_TLS>`, which is activated with the :file:`overlay-native_tls.conf` configuration file.
This command is similar to the modem ``%CMNG`` command, but it utilizes Zephyr setting storage instead of modem credential storage.

.. note::

   The Zephyr setting storage is unencrypted and accessible through the debug port of the nRF91 Series devices.

Set command
-----------

The set command is used for credential storage management.
The command writes and deletes credentials.
It can also list the ``sec_tag`` and ``type`` values of existing credentials.

Syntax
~~~~~~

::

   #XCMNG=<op>[,<sec_tag>[,<type>[,<content>]]]

The ``<op>`` parameter can have the following integer values:

* ``0`` - Write a credential.
* ``1`` - List credentials.
* ``3`` - Delete a credential.

The ``<sec_tag>`` parameter can have an integer value ranging between ``0`` and ``2147483647``.
It is mandatory for *write* and *delete* operations.

The ``<type>`` parameter can have the following integer values:

* ``0`` - Root CA certificate (PEM format)
* ``1`` - Certificate (PEM format)
* ``2`` - Private key (PEM format)
* ``3`` - Pre-shared key (PSK) (ASCII text)
* ``4`` - PSK identity (ASCII text)

It is mandatory for *write* and *delete* operations.

The ``<content>`` parameter can have the following string values:

* The credential in Privacy Enhanced Mail (PEM) format when ``<type>`` has a value of ``0``, ``1`` or ``2``.
* The credential in ASCII text when ``<type>`` has a value of ``3`` or ``4``.

It is mandatory for *write* operations.

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

   AT#XCMNG=0,11,3,"PSK"

   OK

   AT#XCMNG=0,11,4,"Identity"

   OK

   AT#XCMNG=1

   #XCMNG: 11,4
   #XCMNG: 11,3
   #XCMNG: 10,0

   OK

   AT#XCMNG=3,10,0

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
