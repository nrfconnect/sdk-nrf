.. _ug_radio_coex:

Coexistence of short-range radio and other radios
#################################################

.. contents::
   :local:
   :depth: 2

This guide describes how to add short-range radio and other radio coexistence support to your application in |NCS|.

Short-range RF technologies (here referred to as SR), such as Bluetooth® LE or 802.15.4, use a different radio than other technologies like Wi-Fi®  or LTE (here referred to as *the other radios*).
However, if both SR and the other radio attempt to transmit simultaneously, the radio frequency (RF) waves interfere with each other, causing decreased performance and higher power consumption.
Also, in cases like receiving an acknowledgment (ACK), radios should not transmit to ensure correct reception.

These issues are defined as coexistence issues.
To mitigate these issues and to improve performance, a Packet Traffic Arbiter (PTA) is used.
When both SR and the other radio request access to RF, the Packet Traffic Arbiter grants or denies that access.

Integration considerations
**************************

The |NCS| provides the coexistence feature by providing SR protocol drivers that can call the coexistence API and its implementations.
This feature is based on :ref:`nrfxlib:mpsl_cx` provided by the :ref:`nrfxlib:mpsl` (MPSL) library.

The following SR protocols are compatible with the coexistence feature:

   * :ref:`Bluetooth LE <ug_ble_controller>`, if the implementation used is the :ref:`SoftDevice Controller <nrfxlib:softdevice_controller>`.
   * Protocols based on :ref:`nrfxlib:nrf_802154`, such as :ref:`ug_thread` and :ref:`ug_zigbee`.

The following drivers are available:

   * :ref:`ug_radio_mpsl_cx_nrf700x`
   * :ref:`ug_radio_mpsl_cx_generic_3wire`
   * :ref:`ug_radio_mpsl_cx_generic_1wire`
   * :ref:`ug_radio_mpsl_cx_custom`

The |NCS| provides a wrapper that configures Wi-Fi Coexistence based on devicetree source (DTS) and Kconfig information.

The following are the common requirements to use the coexistence feature:

1. The Kconfig option :kconfig:option:`CONFIG_MPSL` must be enabled.
   Some protocol drivers, like :ref:`SoftDevice Controller <nrfxlib:softdevice_controller>`, enable this option by default.
2. The Kconfig option :kconfig:option:`CONFIG_MPSL_CX` must be enabled.
3. The requirements specific to the selected coexistence implementation must be met.
   These include at least selecting one of supported values of the :kconfig:option:`CONFIG_MPSL_CX_CHOICE` choice.
   For details on the requirements specific to the selected coexistence implementation, consult its documentation.
4. Ensure that the configuration of the ``nrf_radio_coex`` node appropriate for the selected implementation is present in the devicetree.
   When using one of the supported implementations, you must use the ``nrf_radio_coex`` name for the node.
   However, if you add a custom user implementation, you can also use a different name.
   Some boards supported by the |NCS| (like :ref:`nrf7002dk <nrf7002dk_nrf5340>`) provide this node by default.
   You can provide the node using either the devicetree source file of the target board or an overlay file.
   See :ref:`zephyr:dt-guide` for more information about the DTS data structure, and :ref:`zephyr:dt_vs_kconfig` for information about differences between DTS and Kconfig.
5. On the nRF5340 SoC, the GPIO pins required for the communication with the PTA must be handed over to the network core.
   You can use the ``nrf-gpio-forwarder`` node in application's core devicetree for that.
6. You can add a new device binding and use it as the ``compatible`` property for the node, if the provided hardware interfaces are unsuitable.

.. note::
   When using the nRF5340, apply steps 1 and 2 only to the network core.
   See :ref:`ug_multi_image`.

.. note::
   Do not enable Wi-Fi coexistence on the nRF5340 SoC in conjunction with Coded Phy and FEM, as this can lead to undefined behavior.

Supported implementations
*************************

The following are the SR protocol driver implementations supported by the |NCS|.

.. _ug_radio_mpsl_cx_nrf700x:

nRF70 Series Wi-Fi coexistence
==============================

The nRF70 Series Wi-Fi coexistence implementation is a three-wire coexistence interface compatible with nRF70 Series devices.

Hardware description
--------------------

The nRF70 Series device interface consists of the signals listed in the table below.
The *Pin* is the pin name of the nRF70 Series device.
The *Direction* is from the point of view of the SoC running the SR protocol.
The *DT property* is the name of the devicetree node property that configures the connection between the SoC running the SR protocol and the nRF70 Series device.

.. table:: nRF70 Series device coexistence protocol pins

   ============  =========  =================================  ==============
   Pin           Direction  Description                        DT property
   ============  =========  =================================  ==============
   COEX_REQ      Out        Request signal                     req-gpios
   COEX_STATUS0  Out        SR transaction direction TX or RX  status0-gpios
   COEX_GRANT    In         Grant                              grant-gpios
   ============  =========  =================================  ==============


Enabling nRF70 Series Wi-Fi coexistence
---------------------------------------

To enable Wi-Fi coexistence on the nRF70 Series device, complete the following steps:

1. Add the following node to the devicetree source file:

   .. code-block::

      / {
            nrf_radio_coex: nrf7002-coex {
               status = "okay";
               compatible = "nordic,nrf700x-coex";
               req-gpios =     <&gpio0 24 (GPIO_ACTIVE_HIGH)>;
               status0-gpios = <&gpio0 14 (GPIO_ACTIVE_HIGH)>;
               grant-gpios =   <&gpio0 25 (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
         };
      };

#. Optionally, replace the node name ``nrf7002-coex`` with a custom one.
#. Replace the pin numbers provided for each of the required properties:

   * ``req-gpios`` - GPIO characteristic of the device that controls the ``COEX_REQ`` signal of the nRF70 Series device.
   * ``status0-gpios`` - GPIO characteristic of the device that controls the ``COEX_STATUS0`` signal of the nRF70 Series device.
   * ``grant-gpios`` - GPIO characteristic of the device that controls the ``COEX_GRANT`` signal of the nRF70 Series device.

   .. note::
      ``GPIO_PULL_UP`` is added to avoid a floating input pin and is required on some boards only.
      If the target board is designed to avoid this signal being left floating, you can remove ``GPIO_PULL_UP`` to save power.


   The ``phandle-array`` type is used, as it is commonly used in Zephyr's devicetree to describe GPIO signals.
   The first element ``&gpio0`` indicates the GPIO port (``port 0`` has been selected in the example shown).
   The second element is the pin number on that port.

#. On the nRF5340, you must also apply the same devicetree node mentioned in step 1 to the network core using sysbuild build system. For more information, see the :ref:`Migrating to sysbuild <child_parent_to_sysbuild_migration>` page.
   To do so, apply the overlay to the correct network core child image by creating an overlay file named :file:`sysbuild/*childImageName*/boards/nrf5340dk_nrf5340_cpunet.overlay.overlay` in your application directory, for example :file:`sysbuild/ipc_radio/boards/nrf5340dk_nrf5340_cpunet.overlay`.

   The ``*childImageName*`` default value is set to ``ipc_radio`` :ref:`ipc_radio`.

   ``ipc_radio`` represents all applications with support for the combination of both 802.15.4 and Bluetooth.
   You can configure your application using the following sysbuild configurations:

   * ``SB_CONFIG_NETCORE_IPC_RADIO=y`` for applications having support for 802.15.4, but not for Bluetooth.
   * ``SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC=y`` for application having support for Bluetooth, but not for 802.15.4.
   * ``SB_CONFIG_NETCORE_IPC_RADIO=y`` and ``SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC=y`` for multiprotocol applications having support for both 802.15.4 and Bluetooth.

#. Enable the following Kconfig options:

   * :kconfig:option:`CONFIG_MPSL_CX`
   * :kconfig:option:`CONFIG_MPSL_CX_NRF700X`

   .. note::
      If a ``nordic,nrf700x-coex`` compatible node is present in the devicetree and :kconfig:option:`CONFIG_MPSL_CX` is set to ``y``, :kconfig:option:`MPSL_CX_NRF700X` will be selected by default.

.. _ug_radio_mpsl_cx_generic_3wire:

Generic three-wire coexistence
==============================

The generic three-wire coexistence is a three-wire coexistence interface which follows the Thread Radio Coexistence Practical recommendations for using a three-wire PTA implementation for co-located 2.4 GHz radios.

Hardware description
--------------------

The generic three-wire interface consists of the signals listed in the table below.
The *Pin* is a generic pin name of a PTA, identified rather by its function.
The *Direction* is from the point of view of the SoC running the SR protocol.
The *DT property* is the name of the devicetree node property that configures the connection between the SoC running the SR protocol and the Wi-Fi device.

.. table:: Generic three-wire coexistence protocol pins

   ============  =========  =================================  ==============
   Pin           Direction  Description                        DT property
   ============  =========  =================================  ==============
   REQUEST       Out        Request signal                     req-gpios
   PRIORITY      Out        Priority signal                    pri-dir-gpios
   GRANT         In         Grant signal                       grant-gpios
   ============  =========  =================================  ==============


Enabling generic three-wire coexistence
---------------------------------------

To enable the generic three-wire coexistence, do the following:


1. Add the following node to the devicetree source file:

   .. code-block::

      / {
            nrf_radio_coex: radio_coex_three_wire {
               status = "okay";
               compatible = "generic-radio-coex-three-wire";
               req-gpios =     <&gpio0 24 (GPIO_ACTIVE_HIGH)>;
               pri-dir-gpios = <&gpio0 14 (GPIO_ACTIVE_HIGH)>;
               grant-gpios =   <&gpio0 25 (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
         };
      };

#. Optionally, replace the node name ``radio_coex_three_wire`` with a custom one.
#. Replace the pin numbers provided for each of the required properties:

   * ``req-gpios`` - GPIO characteristic of the device that controls the ``REQUEST`` signal of the PTA.
   * ``pri-dir-gpios`` - GPIO characteristic of the device that controls the ``PRIORITY`` signal of the PTA.
   * ``grant-gpios`` - GPIO characteristic of the device that controls the ``GRANT`` signal of the PTA (RF medium access granted).

     .. note::
        ``GPIO_PULL_UP`` is added to avoid a floating input pin and is required on some boards only.
        If the target board is designed to avoid this signal being left floating, you can remove ``GPIO_PULL_UP`` to save power.

   The ``phandle-array`` type is used, as it is commonly used in Zephyr's devicetree to describe GPIO signals.
   The first element ``&gpio0`` indicates the GPIO port (``port 0`` has been selected in the example shown).
   The second element is the pin number on that port.

#. On nRF53 devices, you must also apply the same devicetree node mentioned in **Step 1** to the network core using sysbuild.
   To apply the overlay to the correct network core child image, create an overlay file named :file:`sysbuild/*childImageName*/boards/nrf5340dk_nrf5340_cpunet.overlay` in your application directory, for example :file:`sysbuild/ipc_radio/boards/nrf5340dk_nrf5340_cpunet.overlay`.
   For more information, see the :ref:`Migrating to sysbuild <child_parent_to_sysbuild_migration>` page.

   The *childImageName* default value is set to ``ipc_radio``:

   ``ipc_radio`` represents all applications with support for the combination of both 802.15.4 and Bluetooth.
   You can configure your application using the following sysbuild configurations:

   * ``SB_CONFIG_NETCORE_IPC_RADIO=y`` for applications having support for 802.15.4, but not for Bluetooth.
   * ``SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC=y`` for application having support for Bluetooth, but not for 802.15.4.
   * ``SB_CONFIG_NETCORE_IPC_RADIO=y`` and ``SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC=y`` for multiprotocol applications having support for both 802.15.4 and Bluetooth.

#. Enable the following Kconfig options:

   * :kconfig:option:`CONFIG_MPSL_CX`
   * :kconfig:option:`CONFIG_MPSL_CX_3WIRE`

.. _ug_radio_mpsl_cx_generic_1wire:

Generic one-wire coexistence
============================

An example use-case of the generic one-wire coexistence interface is to allow a protocol implementation to coexist alongside an LTE device on a separate chip, such as the nRF91 Series SiP.

Hardware description
--------------------

The generic one-wire interface consists of the signals listed in the table below.
The *Pin* is a generic pin name of a PTA, identified rather by its function.
The *Direction* is from the point of view of the SoC running the coexistence protocol.
The *DT property* is the name of the devicetree node property that configures the connection between the SoC running the coexistence protocol and the other device.

.. table:: Generic one-wire coexistence protocol pins

   ============  =========  =================================  ==============
   Pin           Direction  Description                        DT property
   ============  =========  =================================  ==============
   GRANT         In         Grant signal                       grant-gpios
   ============  =========  =================================  ==============

In cases where the GPIO is asserted after the radio activity has begun, the ``GRANT`` signal triggers a software interrupt, which in turn disables the radio.
No guarantee is made on the latency of this interrupt, but the ISR priority is configurable.

.. note::
   The :ref:`nrfxlib:mpsl` (MPSL) library uses interrupts with priority 0.
   This may delay the GPIOTE interrupt in some rare cases.
   For that reason, it's recommended to deny SR radio activity at least 400 microseconds before activity on the other radio, and to use a GPIOTE interrupt priority as close to 0 as possible.

Enabling generic one-wire coexistence
-------------------------------------

To enable the generic one-wire coexistence, do the following:


1. Add the following node to the devicetree source file:

   .. code-block::

      / {
            nrf_radio_coex: radio_coex_one_wire {
               status = "okay";
               compatible = "generic-radio-coex-one-wire";
               grant-gpios =   <&gpio0 25 GPIO_ACTIVE_LOW>;
               concurrency-mode = <0>;
         };
      };

   The ``concurrency-mode`` property is optional and can be removed.
   By default, or when set to 0, the ``GRANT`` signal will prevent both TX and RX.
   When set to 1, it will only prevent TX.

#. Optionally, if not using the nRF91 Series SiP, the ``GRANT`` signal may be configured active-high using ``GPIO_ACTIVE_HIGH``
#. Optionally, replace the node name ``radio_coex_one_wire`` with a custom one.
#. If not already present in the device tree, the GPIOTE interrupt may additionally be configured as follows (the first element is the IRQn, and the second is the priority):

   .. code-block::

      &gpiote {
        interrupts = < 6 1 >;
      };

#. Replace the pin number provided for the ``grant-gpios`` property:
   This is the GPIO characteristic of the device that interfaces with the ``GRANT`` signal of the PTA (RF medium access granted).

   The first element ``&gpio0`` indicates the GPIO port (``port 0`` has been selected in the example shown).
   The second element is the pin number on that port.

#. Enable the following Kconfig options:

   * :kconfig:option:`CONFIG_MPSL_CX`
   * :kconfig:option:`CONFIG_MPSL_CX_1WIRE`

.. _ug_radio_mpsl_cx_custom:

Custom coexistence implementations
==================================

To add a custom coexistence implementation, complete following steps:

1. Determine the hardware interface of your PTA.
   If your PTA uses an interface different from the ones already provided by the |NCS|, you need to provide a devicetree binding file.
   See :file:`nrf/dts/bindings/radio_coex/generic-radio-coex-three-wire.yaml` file for an example.
#. Extend the Kconfig choice :kconfig:option:`CONFIG_MPSL_CX_CHOICE` with a Kconfig option allowing to select the new coex implementation.
#. Write the implementation for your PTA.
   See the :file:`nrf/subsys/mpsl/cx/3wire/mpsl_cx_3wire.c` file for an example.
   Add the C source file(s) with the implementation, which must contain the following parts:

   * The implementation of the functions required by the interface structure :c:struct:`mpsl_cx_interface_t`.
     Refer to :ref:`MPSL CX API <mpsl_api_sr_cx>` for details.
   * The initialization code initializing the required hardware resources, based on devicetree information.
   * A call to the function :c:func:`mpsl_cx_interface_set()` during the system initialization.

#. Add the necessary CMakeLists.txt entries to get your code compiled when the new Kconfig choice option you added is selected.
