.. _direction_finding_connectionless_tx:

Bluetooth: Direction finding connectionless beacon
##################################################

.. contents::
   :local:
   :depth: 2

The direction finding connectionless beacon sample demonstrates Bluetooth® LE direction finding transmission.

Requirements
************

.. bt_dir_finding_tx_req_start

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires an antenna matrix when operating in angle of departure mode.
It can be a Nordic Semiconductor design 12 patch antenna matrix, or any other antenna matrix.

.. bt_dir_finding_tx_req_end

Overview
********

The direction finding connectionless beacon sample application uses Constant Tone Extension (CTE) that is transmitted with periodic advertising PDUs.

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_ov_start
   :end-before: bt_dir_finding_central_ov_end

Configuration
*************

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_conf_start
   :end-before: bt_dir_finding_central_conf_end

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_5340_conf_start
   :end-before: bt_dir_finding_central_5340_conf_end

.. bt_dir_finding_tx_aoa_mode_start

Angle of arrival mode
=====================

This sample builds the angle of arrival (AoA) mode by default.

.. bt_dir_finding_tx_aoa_mode_end

.. bt_dir_finding_tx_aod_mode_start

Angle of departure mode
=======================

To build this sample with the angle of departure (AoD) mode, set :makevar:`EXTRA_CONF_FILE` to ``overlay-aod.conf;overlay-bt_ll_sw_split.conf`` and set :makevar:`SNIPPET` to ``bt-ll-sw-split`` using the respective :ref:`CMake option <cmake_options>`.

For more information about configuration files in the |NCS|, see :ref:`app_build_system`.

.. bt_dir_finding_tx_aod_mode_end

.. bt_dir_finding_tx_ant_aod_start

Antenna matrix configuration for angle of departure mode
========================================================

To use this sample when AoD mode is enabled, additional configuration of GPIOs is required to control the antenna array.
An example of such configuration is provided in a devicetree overlay file :file:`nrf52833dk_nrf52833.overlay`.

The overlay file provides the information of which GPIOs should be used by the Radio peripheral to switch between antenna patches during the CTE transmission in the AoD mode.
At least two GPIOs must be provided to enable antenna switching.

The GPIOs are used by the Radio peripheral in the order provided by the ``dfegpio#-gpios`` properties.
The order is important because it has an impact on the mapping of the antenna switching patterns to GPIOs (see `Antenna patterns`_).

To successfully use the direction finding beacon when the AoD mode is enabled, provide the following data related to antenna matrix design:

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_conf_list_start
   :end-before: bt_dir_finding_central_conf_list_end

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_ant_pat_start
   :end-before: bt_dir_finding_central_ant_pat_end

.. bt_dir_finding_tx_ant_aod_end

Building and Running
********************
.. |sample path| replace:: :file:`samples/bluetooth/direction_finding_connectionless_tx`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

|test_sample|

1. |connect_terminal_specific|
#. In the terminal window, check for information similar to the following::

      Starting Connectionless Beacon Demo
      Bluetooth initialization...success
      Advertising set create...success
      Update CTE params...success
      Periodic advertising params set...success
      Enable CTE...success
      Periodic advertising enable...success
      Extended advertising enable...success
      Started extended advertising as XX:XX:XX:XX:XX:XX (random)

Dependencies
************

This sample uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`
* :file:`include/sys/util.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/direction.h`
  * :file:`include/bluetooth/gatt.h`
