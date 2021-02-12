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

Set command
-----------

The set command makes the nRF91 development kit go into idle or sleep mode.

Syntax
~~~~~~

::

   #XSLEEP[=<shutdown_mode>]

The ``<shutdown_mode>`` parameter accepts only the following integer values:

* ``0`` - Enter Idle.
  You can also use the syntax ``AT#XSLEEP``.
* ``1`` - Enter Sleep.

The default value is 0.

Response syntax
~~~~~~~~~~~~~~~

There is no response:

* In case of Idle, it will exit by sending data over UART.
* In case of Sleep, it will wake up by GPIO or reset.

Examples
~~~~~~~~

::

   AT#XSLEEP

::

   AT#XSLEEP=0

::

   AT#XSLEEP=1

::

   AT#XSLEEP?
   ERROR

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

   #XSLEEP: (0,1)
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

SLM data mode control #XDATACTL
===============================

The ``#XDATACTRL`` command configures a size or time limit for data mode.

Set command
-----------

The set command configures a size or time limit for data mode.

Syntax
~~~~~~

::

   #XDATACTRL=<size_limit>,<time_limit>

* The ``<size_limit>`` parameter is an integer.
  It indicates the size limit in data mode and accepts any value up to 1024.
* The ``<time_limit>`` parameter is an integer.
  It indicates the time limit (in milliseconds) in data mode and accepts any value up to 10000.

By default, neither a size limit nor a time limit is defined for data mode.

Response syntax
~~~~~~~~~~~~~~~

There is no response.

Example
~~~~~~~

::

   AT#XDATACTRL=1024,5000
   OK

Read command
------------

The read command shows the current size and time configurations for data mode.

Syntax
~~~~~~

::

   AT#XDATACTRL?

Response syntax
~~~~~~~~~~~~~~~

::

   #XDATACTRL: <size_limit>,<time_limit>

* The ``<size_limit>`` parameter is an integer.
  It indicates the configured size limit in data mode.
* The ``<time_limit>`` parameter is an integer.
  It indicates the configured time limit (in milliseconds) in data mode.

Example
~~~~~~~

::

   AT#XDATACTRL?
   #XDATACTRL: 1024,5000
   OK

Test command
------------

The test command tests the existence of the AT command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XDATACTRL=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XDATACTRL: <size_limit>,<time_limit>

Example
~~~~~~~

::

   AT#XDATACTRL=?
   #XDATACTL: <size_limit>,<time_limit>
