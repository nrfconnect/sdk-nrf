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

The ``#XSLEEP`` command makes the nRF9160 development kit enter idle or sleep mode.

If you want to do power measurements on the nRF9160 DK while running the SLM application, disable unused peripherals.

Set command
-----------

The set command makes the nRF9160 development kit enter either idle or sleep mode.

Syntax
~~~~~~

::

   #XSLEEP=<shutdown_mode>

The ``<shutdown_mode>`` parameter accepts only the following integer values:

* ``0`` - Deprecated.
* ``1`` - Enter Sleep.
  In this mode, both the SLM service and the LTE connection are terminated.
  The development kit can be waken up using the :kconfig:option:`CONFIG_SLM_WAKEUP_PIN`.

* ``2`` - Enter Idle.
  In this mode, both the SLM service and the LTE connection are maintained.
  The development kit can be made to exit idle using the :kconfig:option:`CONFIG_SLM_WAKEUP_PIN`.
  If the :kconfig:option:`CONFIG_SLM_INDICATE_PIN` is defined, SLM toggle this GPIO when there is data for MCU.
  MCU could in turn make SLM to exit idle by :kconfig:option:`CONFIG_SLM_WAKEUP_PIN`.
  The data is buffered during the idle status and is sent to MCU after exiting the ilde status.

.. note::
   This parameter does not accept ``0`` anymore.

Examples
~~~~~~~~

::

   AT#XSLEEP=0
   ERROR

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

   #XSLEEP: (1,2)
   OK

SLM UART #XSLMUART
==================

The ``#XSLMUART`` command manages the UART settings.

Set command
-----------

The set command changes the UART baud rate and hardware flow control settings.
Hardware flow control settings can be changed only if :ref:`CONFIG_SLM_UART_HWFC_RUNTIME <CONFIG_SLM_UART_HWFC_RUNTIME>` is selected.
These settings are stored in the flash memory and applied during the application startup.

Syntax
~~~~~~

The following is the syntax when :ref:`CONFIG_SLM_UART_HWFC_RUNTIME <CONFIG_SLM_UART_HWFC_RUNTIME>` is selected:
::

   #XSLMUART[=<baud_rate>,<hwfc>]

The following is the syntax when :ref:`CONFIG_SLM_UART_HWFC_RUNTIME <CONFIG_SLM_UART_HWFC_RUNTIME>` is not selected:
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
If there is no value stored for the variable, it is set to its default value.If not specified , will use previous value.

The ``<hwfc>`` parameter accepts the following integer values:

* ``0`` - Disable UART hardware flow control.

* ``1`` - Enable UART hardware flow control.
  In this mode, SLM configures both the RTS and the CTS pins according to the device-tree file.

Its default value is ``1``.
When not specified, it is set to the last value set for the variable and stored in the flash memory.
If there is no value stored for the variable, it is set to its default value.

Response syntax
~~~~~~~~~~~~~~~

There is no response.

Example
~~~~~~~

::

   AT#XSLMUART=1000000,1
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

   #XSLMUART: <baud_rate>,<hwfc>

Example
~~~~~~~

::

   AT#XSLMUART?
   #XSLMUART: 115200,1
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

The following is the syntax when :ref:`CONFIG_SLM_UART_HWFC_RUNTIME <CONFIG_SLM_UART_HWFC_RUNTIME>` is selected:

::

   #XSLMUART: (list of the available baud rate options),(disable or enable hwfc)

The following is the syntax when :ref:`CONFIG_SLM_UART_HWFC_RUNTIME <CONFIG_SLM_UART_HWFC_RUNTIME>` not selected:

::

   #XSLMUART: (list of the available baud rate options)

Example
~~~~~~~

The following is an example when :ref:`CONFIG_SLM_UART_HWFC_RUNTIME <CONFIG_SLM_UART_HWFC_RUNTIME>` is selected:

::

   AT#XSLMUART=?
   #XSLMUART: (1200,2400,4800,9600,14400,19200,38400,57600,115200,230400,460800,921600,1000000),(0,1)

The following is an example when :ref:`CONFIG_SLM_UART_HWFC_RUNTIME <CONFIG_SLM_UART_HWFC_RUNTIME>` is not selected:

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
