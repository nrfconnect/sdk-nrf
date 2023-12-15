.. _SLM_AT_GPIO:

GPIO AT commands
****************

.. contents::
   :local:
   :depth: 2

This page describes AT commands related to General Purpose Input/Output (GPIO).

Configure GPIO pins function #XGPIOCFG
======================================

The ``#XGPIOCFG`` command configures the specified GPIO function.

Set command
-----------

The set command allows you to list all available GPIO instances.

Syntax
~~~~~~

::

   #XGPIOCFG=<op>,<pin>

The ``<op>`` parameter indicates the function to be configured.
It accepts the following integer values:

   * ``0`` - Disable GPIO.
   * ``1`` - Output.
   * ``21`` - Input using internal pull up register.
   * ``22`` - Input using internal pull down register.

The ``<pin>`` parameter indicates the GPIO pin to be configured.
It ranges between ``0`` and ``31``.

Example
~~~~~~~

Configure GPIO pin 2 as output:

::

   AT#XGPIOCFG=1,2
   OK

Configure GPIO pin 6 as input pull up:

::

   AT#XGPIOCFG=21,6
   OK

Read command
------------

The read command lists any GPIOs configured by the #XGPIOCFG command.

Example
~~~~~~~

::

    AT#XGPIOCFG?

    #XGPIOCFG

    1,2

    21,6
    OK

Test command
------------

The test command is not supported.


Access GPIO pins #XGPIO
=======================

The ``#XGPIO`` command writes, reads, or toggles GPIO pins state.

Set command
-----------

The set command allows you to write, read, or toggle GPIO pins state.

Syntax
~~~~~~

::

   #XGPIO=<op>,<pin>[,<value>]

* The ``<op>`` parameter accepts the following integer values:

  * ``0`` - Write output GPIO state.
  * ``1`` - Read input GPIO state.
  * ``2`` - Toggle output GPIO state.

* The ``<pin>`` parameter indicates the GPIO pin to be accessed.
  It ranges between ``0`` and ``31``.

* The ``<value>`` parameter indicates the value to be written to the GPIO pin.
  It accepts one of the following values:

  * ``0`` - Logic low.
  * ``1`` - Logic high.

Response syntax
~~~~~~~~~~~~~~~

Response example when ``<op>`` values are write and toggle:

::

   OK

Response example when the ``<op>`` value is read:

::

   #XGPIO: <pin>,<value>

Example
~~~~~~~

Example of a write operation:

::

   AT#XGPIO=0,2,1
   OK

Example of a read operation:

::

   AT#XGPIO=1,2

   #XGPIO: 2,1

   OK

Example of a toggle operation:

::

   AT#XGPIO=2,2
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
