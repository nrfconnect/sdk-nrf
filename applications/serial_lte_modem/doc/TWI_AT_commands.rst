.. _SLM_AT_TWI:

TWI AT commands
***************

.. contents::
   :local:
   :depth: 2

The following commands list contains AT commands related to the two-wire interface (TWI).

List TWI instances #XTWILS
==========================

The ``#XTWILS`` command lists all available TWI instances.

Set command
-----------

The set command allows you to list all available TWI instances.

Response syntax
~~~~~~~~~~~~~~~

::

   #XTWILS: <index>[[[,<index>],<index>],<index>]

The ``<index>`` parameter corresponds to the following TWI instances:

* ``0`` - TWI0 (``i2c0``).
* ``1`` - TWI1 (``i2c1``).
* ``2`` - TWI2 (``i2c2``).
* ``3`` - TWI3 (``i2c3``).

  See :ref:`zephyr:dtbinding_nordic_nrf_twi`.

Example
~~~~~~~

::

   AT#XTWILS
   #XTWILS: 2
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Write to TWI peripheral device #XTWIW
=====================================

The ``#XTWIW`` command writes data to a TWI peripheral device.

Set command
-----------

The set command allows you to write data to a TWI peripheral device.

Syntax
~~~~~~

::

   #XTWIW=<index>,<dev_addr>,<data>

* The ``<index>`` parameter accepts the following integer values:

  * ``0`` - Use TWI0 (``i2c0``).
  * ``1`` - Use TWI1 (``i2c1``).
  * ``2`` - Use TWI2 (``i2c2``).
  * ``3`` - Use TWI3 (``i2c3``).

  See :ref:`zephyr:dtbinding_nordic_nrf_twi`.

* The ``<dev_addr>`` parameter is a hexadecimal string.
  It represents the peripheral device address to write to.
  The maximum length is 2 characters (for example, "DE" for 0xDE).
* The ``<data>`` parameter is a hexadecimal string.
  It represents the data to be written to the peripheral device.
  The maximum length is 255 characters (for example, "DEADBEEF" for 0xDEADBEEF).

Response syntax
~~~~~~~~~~~~~~~

There is no response.

Example
~~~~~~~

::

   AT#XTWIW=2,"76","D0"
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Read from TWI peripheral device #XTWIR
======================================

The ``#XTWIR`` command reads data from a TWI peripheral device.

Set command
-----------

The set command allows you to read data from a TWI peripheral device.

Syntax
~~~~~~

::

   #XTWIR=<index>,<dev_addr>,<num_read>

* The ``<index>`` parameter accepts the following integer values:

  * ``0`` - Use TWI0 (``i2c0``).
  * ``1`` - Use TWI1 (``i2c1``).
  * ``2`` - Use TWI2 (``i2c2``).
  * ``3`` - Use TWI3 (``i2c3``).

* The ``<dev_addr>`` parameter is a hexadecimal string.
  It represents the peripheral device address to read from.
  The maximum length is 2 characters (for example, "DE" for 0xDE).
* The ``<num_read>`` parameter is an unsigned 8-bit integer.
  It represents the amount of data to read from the peripheral device.
  The available range is from 0 to 255 bytes.

Response syntax
~~~~~~~~~~~~~~~

::

   #XTWIR:
   <data>

* The ``<data>`` parameter is a hexadecimal string.
  It represents the data read from the peripheral device.

Example
~~~~~~~

::

   AT#XTWIR=2,"76",1

   #XTWIR: 61
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Write data and read from TWI peripheral device #XTWIWR
======================================================

The ``#XTWIWR`` command writes data to a TWI peripheral device and then reads data from the device.

Set command
-----------

The set command allows you to first write data to a TWI peripheral device and then read the returned data.

Syntax
~~~~~~

::

   #XTWIW=<index>,<dev_addr>,<data>,<num_read>

* The ``<index>`` parameter accepts the following integer values:

  * ``0`` - Use TWI0 (``i2c0``).
  * ``1`` - Use TWI1 (``i2c1``).
  * ``2`` - Use TWI2 (``i2c2``).
  * ``3`` - Use TWI3 (``i2c3``).

  See :ref:`zephyr:dtbinding_nordic_nrf_twi`.

* The ``<dev_addr>`` parameter is a hexadecimal string.
  It represents the peripheral device address to write to.
  The maximum length is 2 characters (for example, "DE" for 0xDE).
* The ``<data>`` parameter is a hexadecimal string.
  It represents the data to be written to the peripheral device.
  The maximum length is 255 characters (for example, "DEADBEEF" for 0xDEADBEEF).
* The ``<num_read>`` parameter is an unsigned 8-bit integer.
  It represents the amount of data to read from the peripheral device.
  The available range is from 0 to 255 bytes.

Response syntax
~~~~~~~~~~~~~~~

::

   #XTWIWR:
   <data>

* The ``<data>`` parameter is a hexadecimal string.
  It represents the data read from the peripheral device.

Example
~~~~~~~

::

   AT#XTWIWR=2,"76","D0",1

   #XTWIWR: 61
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
