.. _SLM_AT_TWI:

TWI AT commands
***************

.. contents::
   :local:
   :depth: 2

This page describes AT commands related to the Two-Wire Interface (TWI).

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

  See :dtcompatible:`nordic,nrf-twi`.

Example
~~~~~~~

The following example is meant for Thingy:91.

It shows that TWI2 (``i2c2``) is available.
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

  See :dtcompatible:`nordic,nrf-twi`.

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

The following example is meant for Thingy:91.

It performs a write operation to the device address ``0x76`` (BME680), and it writes ``D0`` to the device.
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

The following example is meant for Thingy:91.

It performs a read operation to the device address ``0x76`` (BME680), and it reads 1 byte from the device.
The value returned (``61``) indicates ``0x61`` as the ``CHIP ID``.
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

  See :dtcompatible:`nordic,nrf-twi`.

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

Examples
~~~~~~~~

* The following example is meant for Thingy:91.

  It performs a write-then-read operation to the device address ``0x76`` (BME680) to get the ``CHIP ID`` of the device.
  The value returned (``61``) indicates ``0x61`` as the ``CHIP ID``.

  ::

     AT#XTWIWR=2,"76","D0",1

     #XTWIWR: 61
     OK

* The following example is meant for Thingy:91.

  It performs a write-then-read operation to the device address ``0x38`` (BH1749) to get the ``MANUFACTURER ID`` of the device.
  The value returned (``E0``) indicates ``0xE0`` as the ``MANUFACTURER ID`` of the device.

  ::

     AT#XTWIWR=2,"38","92",1

     #XTWIWR: E0
     OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
