.. _gzp:

Gazell Pairing
##############

.. contents::
   :local:
   :depth: 2

The Gazell pairing library enables applications to use the Gazell Link Layer to provide a secure wireless link between Gazell nodes.
The library is customized for pairing a Device (for example, a mouse, keyboard, or remote control) with a Host (typically a USB dongle) using Gazell.

Overview
********

Gazell Pairing determines the channel set used by Gazell.
See the :ref:`ug_gzp` user guide for more information, such as the features of this library.

This library is used in the :ref:`gzp_dynamic_pairing_host` and :ref:`gzp_dynamic_pairing_device` samples.'

Requirements
************

The Gazell Pairing library requires the same resources as the :ref:`Gazell Link Layer <ug_gzll_resources>`.

In addition, the Gazell Pairing library also employs three nRF52 Series peripherals:

* Random Number Generator, for generating keys and tokens.
* AES Electronic Codebook (ECB), for encryption and decryption.
* Non-Volatile Memory Controller (NVMC), for storing of pairing parameters.

In addition, Gazell Pairing requires the Gazell Link Layer resource of two pipes: one for pairing and one for encrypted data transmission.

Since Gazell Pairing requires exclusive access to pipes 0 and :c:macro:`GZP_DATA_PIPE` (default pipe 1), it must control the internal Gazell Link Layer variables ``base_address_0``, ``base_address_1`` and ``prefix_address_byte`` for pipes :c:macro:`GZP_PAIRING_PIPE` (always pipe 0) and :c:macro:`GZP_DATA_PIPE` (configurable).

* The main application can use the pipes 2-7.
* The ``base_address_1`` applies to these pipes.
* Gazell Pairing must also determine whether the RX pipes 0 and 1 are enabled.

.. note::
   Make sure not to affect the ``rx_enabled`` status of these pipes.

Do not access the following:

* :c:func:`nrf_gzll_set_base_address_0()`
* :c:func:`nrf_gzll_set_base_address_1()`
* :c:func:`nrf_gzll_set_address_prefix_byte()` (not for pipes 0 and 1)
* :c:func:`nrf_gzll_set_rx_pipes_enabled()` (can be used but the enabled status of pipes 0 and 1 should not be modified)
* :c:func:`nrf_gzll_set_channel_table()`

Configuration
*************

Complete the following steps for configuration:

1. The prerequisite :ref:`ug_gzll` should be enabled as described in the Gazell Link Layer :ref:`ug_gzll_configuration` section.
#. Set the :kconfig:option:`CONFIG_GAZELL_PAIRING` Kconfig option to enable the Gazell Pairing.
#. Select the role by either of the following Kconfig options:

   a. :kconfig:option:`CONFIG_GAZELL_PAIRING_DEVICE` - Device.
   #. :kconfig:option:`CONFIG_GAZELL_PAIRING_HOST` - Host.

To support persistent storage of pairing data, set the :kconfig:option:`CONFIG_GAZELL_PAIRING_SETTINGS` Kconfig option.

To support encryption, set the :kconfig:option:`CONFIG_GAZELL_PAIRING_CRYPT` Kconfig option.

API documentation
*****************

| Header file: :file:`include/gzp.h` and :file:`include/gzp_config.h`
| Source file: :file:`subsys/gazell/`

.. doxygengroup:: gzp
   :project: nrf
   :members:
