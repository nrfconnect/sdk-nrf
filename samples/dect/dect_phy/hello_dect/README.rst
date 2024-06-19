.. _nrf_modem_dect_phy_hello:

nRF91x1: DECT NR+ PHY hello
###########################

.. contents::
   :local:
   :depth: 2

The DECT NR+ physical layer (PHY) hello sample demonstrates how to set up a simple DECT NR+ application with the DECT PHY firmware.

.. important::

   The sample only showcases the use of the :ref:`nrf_modem_dect_phy` interface of the Modem library and is not a complete standalone application.

Requirements
************

The sample supports the following development kits, and requires at least two kits:

.. table-from-sample-yaml::

.. note::

   The :ref:`nrf_modem_dect_phy_hello` sample requires the DECT NR+ PHY firmware to run on the nRF91x1 modem core.
   For more information, contact the Nordic Semiconductor sales department.

.. include:: /includes/tfm.txt

Overview
********

The sample shows a simple broadcast and reception of DECT NR+ messages between devices on a hard-coded channel.

After initialization, the devices run into a loop, transmitting a counter value before listening for incoming receptions.
The time to listen is set by the :ref:`CONFIG_RX_PERIOD_S <CONFIG_RX_PERIOD_S>` Kconfig option.
The loop is exited after a number of transmissions set by the :ref:`CONFIG_TX_TRANSMISSIONS <CONFIG_TX_TRANSMISSIONS>` Kconfig option, or it continues forever if the :ref:`CONFIG_TX_TRANSMISSIONS <CONFIG_TX_TRANSMISSIONS>` Kconfig option is set to ``0``.

Each device can be reset to run the sample again.

Configuration
**************

|config|

Configuration options
=====================

Check and configure the following Kconfig options:

.. _CONFIG_CARRIER:

CONFIG_CARRIER
   The sample configuration defines the carrier to use.
   The availability of the channels and the exact regulations for using them vary in different countries.
   See section 5.4.2 of `ETSI TS 103 636-2`_ for the calculation.

.. _CONFIG_NETWORK_ID:

CONFIG_NETWORK_ID
   The configuration option specifies the network ID.
   It ranges from ``1`` to ``4294967295`` with the default value set to ``91``.

.. _CONFIG_MCS:

CONFIG_MCS
   The configuration option specifies the :term:`Modulation Coding Scheme (MCS)`.
   The MCS impacts how much data can fit into each subslot.
   The default value is set to ``1``.

.. _CONFIG_TX_POWER:

CONFIG_TX_POWER
   The configuration option sets the transmission power.
   See the table 6.2.1-3 of `ETSI TS 103 636-4`_ for more details.
   It ranges from ``0`` to ``11`` with the default value set to ``11``.

.. _CONFIG_TX_TRANSMISSIONS:

CONFIG_TX_TRANSMISSIONS
   The configuration option sets the number of transmissions before the sample exits.
   It ranges from ``0`` to ``4294967295`` with the default value set to ``30``.

.. _CONFIG_RX_PERIOD_S:

CONFIG_RX_PERIOD_S
   The sample configuration sets the receive window period.
   The time is set in seconds.
   The default value is set to 5 seconds.

Building and running
********************

.. |sample path| replace:: :file:`samples/dect/dect_phy/hello_dect`

.. include:: /includes/build_and_run_ns.txt

.. important::

   DECT NR+ operates on free but regulated radio channels.
   The regulations and availability of the channels vary by country and region.
   It is your responsibility to operate the devices according to local regulations, both at the development site and in the device operating regions.
   If you are in the EU or US with permission to operate on the DECT band and can ensure that access rules are met, you can use the ``overlay-eu.conf`` and ``overlay-us.conf`` Kconfig overlays, respectively.
   If other configuration is required, set the carrier using the :ref:`CONFIG_CARRIER <CONFIG_CARRIER>` Kconfig option, and the transmission power using the :ref:`CONFIG_TX_POWER <CONFIG_TX_POWER>` Kconfig option.
   If you, as a user, are not permitted to operate on the DECT band or cannot ensure that access rules are met, you must ensure there are no radio emissions from the devices running the sample.
   This can be done with the use of an RF chamber.

   * To build with one of the provided overlays, run the following command:

      .. parsed-literal::
         :class: highlight

         west build -p -b *build_target* -- -DEXTRA_CONF_FILE="<your_overlay>"

   * To build with other carrier and transmission power configuration, run the following command:

      .. parsed-literal::
         :class: highlight

         west build -p -b *build_target* -- -DCONFIG_CARRIER=<your_carrier> -DCONFIG_TX_POWER=<your_tx_power>

   You can also add your own overlay or the Kconfig options to the project configuration to build with other carrier and transmission power configurations.

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Observe that the devices transmit a counter value that is received by the other devices.
#. After a given number of transmissions, observe that the devices shut down and exit the sample.

Sample output
=============

The sample shows an output similar to the following:

Device 1:

.. code-block:: console

   *** Booting nRF Connect SDK v3.5.99-ncs1 ***
   [00:00:00.378,784] <inf> app: Dect NR+ PHY Hello sample started
   [00:00:00.691,375] <inf> app: Dect NR+ PHY initialized, device ID: 12345
   [00:00:00.691,406] <inf> app: Transmitting 0
   [00:00:05.697,784] <inf> app: Transmitting 1
   [00:00:10.704,193] <inf> app: Transmitting 2
   [00:00:14.186,553] <inf> app: Received header from device ID 67890
   [00:00:14.186,889] <inf> app: Received data (RSSI: -54.5): Hello DECT! 0
   [00:00:15.710,571] <inf> app: Transmitting 3
   [00:00:19.192,932] <inf> app: Received header from device ID 67890
   [00:00:19.193,267] <inf> app: Received data (RSSI: -54.5): Hello DECT! 1
   [00:00:20.716,949] <inf> app: Transmitting 4
   ...
   [00:02:24.352,661] <inf> app: Received header from device ID 67890
   [00:02:24.352,996] <inf> app: Received data (RSSI: -54.5): Hello DECT! 26
   [00:02:25.876,739] <inf> app: Transmitting 29
   [00:02:25.876,831] <inf> app: Reached maximum number of transmissions (30)
   [00:02:25.876,831] <inf> app: Shutting down
   [00:02:25.893,554] <inf> app: Bye!

Device 2:

.. code-block:: console

   *** Booting nRF Connect SDK v3.5.99-ncs1 ***
   [00:00:00.407,287] <inf> app: Dect NR+ PHY Hello sample started
   [00:00:00.719,238] <inf> app: Dect NR+ PHY initialized, device ID: 67890
   [00:00:00.719,268] <inf> app: Transmitting 0
   [00:00:02.254,211] <inf> app: Received header from device ID 12345
   [00:00:02.254,547] <inf> app: Received data (RSSI: -54.5): Hello DECT! 3
   [00:00:05.725,646] <inf> app: Transmitting 1
   [00:00:07.260,620] <inf> app: Received header from device ID 12345
   [00:00:07.260,955] <inf> app: Received data (RSSI: -54.5): Hello DECT! 4
   ...
   [00:02:10.885,284] <inf> app: Transmitting 26
   [00:02:12.420,318] <inf> app: Received header from device ID 12345
   [00:02:12.420,654] <inf> app: Received data (RSSI: -54.5): Hello DECT! 29
   [00:02:15.891,693] <inf> app: Transmitting 27
   [00:02:20.898,071] <inf> app: Transmitting 28
   [00:02:25.904,449] <inf> app: Transmitting 29
   [00:02:25.904,541] <inf> app: Reached maximum number of transmissions (30)
   [00:02:25.904,571] <inf> app: Shutting down
   [00:02:25.921,325] <inf> app: Bye!

Dependencies
************

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`zephyr:uart_api`


In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
