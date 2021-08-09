.. _SLM_AT_gen:

Generic AT commands
*******************

.. contents::
   :local:
   :depth: 2

The following commands list contains generic AT commands.

SLM version #XSLMVER
====================

The ``#XSLMVER`` command requests the SLM version.

Set command
-----------

The set command requests the SLM version.

Syntax
~~~~~~

::

   #XSLMVER

Response syntax
~~~~~~~~~~~~~~~

::

   #XSLMVERSION: <version>

The ``<version>`` value returns a string containing the SLM version.

Example
~~~~~~~

The following command example reads the SLM version:

::

   AT#XSLMVER
   #XSLMVER: 1.2
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

   at#xclac
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
   AT#XGPSRUN
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Power saving #XSLEEP
====================

The ``#XSLEEP`` command makes the nRF91 development kit go into idle or sleep mode.

If you are going to do power measurements on the nRF9160 DK while running the SLM application it is recommended to disable unused peripherals.
By default UART2 is enabled in the :file:`nrf9160dk_nrf9160_ns.overlay` file so disable the UART2 by switching the status.
Change the line ``status = "okay"`` to ``status = "disabled"`` and then save the :file:`nrf9160dk_nrf9160_ns.overlay`` file to make sure you will get the expected power consumption numbers when doing the measurements.

Set command
-----------

The set command makes the nRF91 development kit go into either idle or sleep mode, or it powers off the UART device.

Syntax
~~~~~~

::

   #XSLEEP=<shutdown_mode>

The ``<shutdown_mode>`` parameter accepts only the following integer values:

* ``0`` - Enter Idle.
  In this mode, the SLM service is terminated, but the LTE connection is maintained.
* ``1`` - Enter Sleep.
  In this mode, both the SLM service and the LTE connection are terminated.
* ``2`` - Power off UART.
  In this mode, both the SLM service and the LTE connection are maintained.

* In case of Idle, it will exit by interface GPIO.
* In case of Sleep, it will wake up by interface GPIO.
* In case of UART power off, UART will be powered on by interface GPIO or internally by SLM when needed.

Examples
~~~~~~~~

::

   AT#XSLEEP=0
   OK

::

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

   #XSLEEP: (0,1,2)
   OK

SLM UART #XSLMUART
==================

The ``#XSLMUART`` command manages the UART settings.

Set command
-----------

The set command changes the UART settings.

Syntax
~~~~~~

::

   #XSLMUART[=<baud_rate>]

The ``<baud_rate>`` parameter is an integer.
It accepts only the following values:

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

The default value is ``115200``.

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

   AT#XSLMUART?

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
