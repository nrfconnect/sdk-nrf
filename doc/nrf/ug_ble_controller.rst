.. _ug_ble_controller:

Bluetooth LE Controller
#######################

When you develop a Bluetooth Low Energy (LE) application, you must include a Bluetooth LE Controller.
A Bluetooth LE Controller is the layer of the Bluetooth stack that handles the physical layer packets and all associated timing.
It implements the Link Layer, which is the low-level, real-time protocol that controls the Bluetooth LE communication.

The |NCS| contains two implementations of a Bluetooth LE Controller:

* `nRF Bluetooth LE Controller`_
* `Zephyr Bluetooth LE Controller`_

The nRF Bluetooth LE Controller is implemented by Nordic Semiconductor.
The Zephyr Bluetooth LE Controller is an open source Link Layer developed in the `Zephyr Project`_, to which Nordic Semiconductor is a contributor.

Both Link Layers integrate with the Zephyr Bluetooth LE Host, which completes the full Bluetooth LE protocol stack solution in the |NCS|.
It is possible to select either Bluetooth LE Controller for application development.
See `Usage in samples`_ for more information.


nRF Bluetooth LE Controller
***************************

The :ref:`nRF Bluetooth LE Controller <nrfxlib:ble_controller>` is designed for nRF52 and nRF53 Series devices.
It provides the same implementation of the Link Layer that is available as part of Nordic Semiconductor's SoftDevices.
The nRF Bluetooth LE Controller is developed, tested, and supported by Nordic Semiconductor.

The nRF Bluetooth LE Controller is distributed as a set of precompiled, linkable libraries that can be found in the `nrfxlib`_ repository.
There are different variants of the libraries that support different feature sets.
Which variant you should choose depends on the chip that you are using, the features that you need, and the amount of available memory.

Nordic's Bluetooth LE Controller supports an extensive standard feature set from the Bluetooth 5.2 specification and a number of extensions for high-performance applications like Low Latency Packet mode (LLPM).
See the :ref:`nRF Bluetooth LE Controller documentation <nrfxlib:ble_controller>` for a detailed list of supported features.


Zephyr Bluetooth LE Controller
******************************

The `Zephyr Project`_ has a full, open source, :ref:`Bluetooth Low Energy stack <zephyr:bluetooth-arch>`, which includes the Zephyr Bluetooth LE Controller.
This Bluetooth LE Controller is designed with an upper and lower implementation to make it possible to support multiple hardware platforms.
It is developed by the Zephyr community and provided as open source.

To use Zephyr's Bluetooth LE Controller in your application, include a :ref:`Controller-only build <zephyr:bluetooth-build-types>` of the Bluetooth LE stack.

Zephyr's Bluetooth LE Controller supports most of the standard Bluetooth LE features, except for Advertising Extensions.
See the :ref:`Zephyr documentation <zephyr:bluetooth-overview>` for a detailed list of supported features.


Usage in samples
****************

Most :ref:`Bluetooth LE samples <ble_samples>` in the |NCS| can use either Bluetooth LE Controller.
An exception is the :ref:`ble_llpm` sample, which requires the nRF Bluetooth LE Controller that supports LLPM.

By default, all samples except for the :ref:`ble_llpm` sample are currently configured to use Zephyr's Bluetooth LE Controller.
To use the nRF Bluetooth LE Controller instead, set :option:`CONFIG_BT_LL_NRFXLIB_DEFAULT` to ``y`` in the :file:`prj.conf` file (see :ref:`configure_application`) and make sure to build from a clean build directory.
