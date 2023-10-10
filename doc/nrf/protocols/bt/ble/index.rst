.. _ug_ble_controller:

Bluetooth LE Controller
#######################

.. contents::
   :local:
   :depth: 2

When you develop a Bluetooth® Low Energy (LE) application, you must include a Bluetooth® LE Controller.
A Bluetooth LE Controller is the layer of the Bluetooth stack that handles the physical layer packets and all associated timing.
It implements the Link Layer, which is the low-level, real-time protocol that controls the Bluetooth LE communication.

The |NCS| contains two implementations of a Bluetooth LE Controller:

* `SoftDevice Controller`_
* `Zephyr Bluetooth LE Controller`_

The SoftDevice Controller is implemented by Nordic Semiconductor.
The Zephyr Bluetooth LE Controller is an open source Link Layer developed in `Zephyr`_, to which Nordic Semiconductor is a contributor.

Both Link Layers integrate with the Zephyr Bluetooth Host, which completes the full Bluetooth LE protocol stack solution in the |NCS|.
It is possible to select either Bluetooth LE Controller for application development.
See `Usage in samples`_ for more information.

If you want to go through an online training course to familiarize yourself with Bluetooth Low Energy and the development of Bluetooth LE applications, enroll in the `Bluetooth LE Fundamentals course`_ in the `Nordic Developer Academy`_.

.. _ug_ble_controller_softdevice:

SoftDevice Controller
*********************

The :ref:`SoftDevice Controller <nrfxlib:softdevice_controller>` is designed for nRF52 and nRF53 Series devices.
It provides the same implementation of the Link Layer that is available as part of Nordic Semiconductor's SoftDevices.
The SoftDevice Controller is developed, tested, and supported by Nordic Semiconductor.

The SoftDevice Controller is distributed as a set of precompiled, linkable libraries that can be found in the `sdk-nrfxlib`_ repository.
There are different variants of the libraries that support different feature sets.
Which variant you should choose depends on the chip that you are using, the features that you need, and the amount of available memory.

Nordic's SoftDevice Controller supports an extensive standard feature set from the Bluetooth® specification and a number of extensions for high-performance applications like Low Latency Packet mode (LLPM).
See the :ref:`SoftDevice Controller documentation <nrfxlib:softdevice_controller>` for a detailed list of supported features.


Zephyr Bluetooth LE Controller
******************************

`Zephyr`_ has a full, open source, :ref:`Bluetooth Low Energy stack <zephyr:bluetooth-arch>`, which includes the Zephyr Bluetooth LE Controller.
This Bluetooth LE Controller is designed with an upper and lower implementation to make it possible to support multiple hardware platforms.
It is developed by the Zephyr community and provided as open source.

To use Zephyr's Bluetooth LE Controller in your application, include a :ref:`Controller-only build <zephyr:bluetooth-build-types>` of the Bluetooth LE stack.

Zephyr's Bluetooth LE Controller supports most of the standard Bluetooth LE features.
See the :ref:`Zephyr documentation <zephyr:bluetooth-overview>` for a detailed list of supported features.


Usage in samples
****************

Most :ref:`Bluetooth LE samples <ble_samples>` in the |NCS|, including the :ref:`bt_mesh_samples`, can use either Bluetooth LE Controller.
An exception is the :ref:`ble_llpm` sample, which requires the SoftDevice Controller that supports LLPM.
There is also a separate controller for the :ref:`nrf53_audio_app` application, namely the :ref:`lib_bt_ll_acs_nrf53_readme`.

By default, all samples are currently configured to use SoftDevice Controller.
To use the Zephyr Bluetooth LE Controller instead, set :kconfig:option:`CONFIG_BT_LL_SW_SPLIT` to ``y`` in the :file:`prj.conf` file (see :ref:`configure_application`) and make sure to build from a clean build directory.

.. note::
   If your Bluetooth application requires the LE Secure Connections pairing and you want to use the Zephyr Bluetooth LE Controller, make sure to enable the :kconfig:option:`CONFIG_BT_TINYCRYPT_ECC` option as the ECDH cryptography is not supported by this Bluetooth LE Controller.
