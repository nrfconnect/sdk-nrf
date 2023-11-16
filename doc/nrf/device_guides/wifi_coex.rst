.. _ug_radio_coex:

Coexistence of short-range radio and other radios
#################################################

.. contents::
   :local:
   :depth: 2

This guide describes how to add short-range radio and other radio coexistence support to your application in |NCS|.

Short-range RF technologies (here referred to as SR), such as Bluetooth LE or 802.15.4, use a different radio than other technologies like Wi-Fi or LTE (here referred to as *the other radios*).
However, if both SR and the other radio attempt to transmit simultaneously, the radio frequency (RF) waves interfere with each other, causing decreased performance and higher power consumption.
Also, in cases like receiving an acknowledgment (ACK), radios should not transmit to ensure correct reception.

These issues are defined as coexistence issues.
To mitigate these issues and to improve performance, a Packet Traffic Arbiter (PTA) is used.
When both SR and the other radio request access to RF, the Packet Traffic Arbiter grants or denies that access.

Integration considerations
**************************

The |NCS| provides the coexistence feature by providing SR protocol drivers that can call the coexistence API and its implementations.
The coexistence feature is provided in following ways:

   * :ref:`ug_radio_coex_mpsl_cx_based`
   * :ref:`ug_radio_coex_bluetooth_only_based`


.. _ug_radio_coex_mpsl_cx_based:

Coexistence based on MPSL CX API
================================

This coexistence feature is based on :ref:`nrfxlib:mpsl_cx` provided by the :ref:`nrfxlib:mpsl` (MPSL) library.
It is recommended for new designs.

The following SR protocols are compatible with the coexistence based on MPSL CX API:

   * :ref:`Bluetooth LE <ug_ble_controller>`, if the implementation used is the :ref:`SoftDevice Controller <nrfxlib:softdevice_controller>`.
   * Protocols based on :ref:`nrfxlib:nrf_802154`, such as :ref:`ug_thread` and :ref:`ug_zigbee`.

This way of providing coexistence applies to the following implementations:

   * :ref:`ug_radio_mpsl_cx_nrf700x`
   * :ref:`ug_radio_mpsl_cx_generic_3wire`
   * :ref:`ug_radio_mpsl_cx_custom`

The |NCS| provides a wrapper that configures Wi-Fi Coexistence based on devicetree source (DTS) and Kconfig information.

The following are the common requirements to use coexistence based on the MPSL CX API:

1. The Kconfig option :kconfig:option:`CONFIG_MPSL` must be enabled.
   Some protocol drivers, like :ref:`SoftDevice Controller <nrfxlib:softdevice_controller>`, enable this option by default.
2. The Kconfig option :kconfig:option:`CONFIG_MPSL_CX` must be enabled.
3. The requirements specific to the selected coexistence implementation must be met.
   These include at least selecting one of supported values of the :kconfig:option:`CONFIG_MPSL_CX_CHOICE` choice.
   For details on the requirements specific to the selected coexistence implementation, consult its documentation.
4. Ensure that the configuration of the ``nrf_radio_coex`` node appropriate for the selected implementation is present in the devicetree.
   When using one of the supported implementations, you must use the ``nrf_radio_coex`` name for the node.
   However, if you add a custom user implementation, you can also use a different name.
   Some boards supported by the |NCS| (like :ref:`nrf7002dk_nrf5340 <nrf7002dk_nrf5340>`) provide this node by default.
   You can provide the node using either the devicetree source file of the target board or an overlay file.
   See :ref:`zephyr:dt-guide` for more information about the DTS data structure, and :ref:`zephyr:dt_vs_kconfig` for information about differences between DTS and Kconfig.
5. On the nRF5340 SoC, the GPIO pins required for the communication with the PTA must be handed over to the network core.
   You can use the ``nrf-gpio-forwarder`` node in application's core devicetree for that.
6. You can add a new device binding and use it as the ``compatible`` property for the node, if the provided hardware interfaces are unsuitable.

.. note::
   When using the nRF5340, apply steps 1 and 2 only to the network core.
   See :ref:`ug_multi_image`.

.. _ug_radio_coex_bluetooth_only_based:

MPSL provided Bluetooth-only coexistence
========================================

The MPSL-provided Bluetooth-only coexistence can be used only with the :ref:`ug_radio_coex_bluetooth_only_1wire` implementation.


It is based on :ref:`nrfxlib:bluetooth_coex` provided by the :ref:`nrfxlib:mpsl` (MPSL) library.


Supported implementations
*************************

The following are the implementations supported by the MPSL-provided Bluetooth-only coexistence.

.. note::
   Do not enable Wi-Fi coexistence on the nRF5340 SoC in conjunction with Coded Phy and FEM, as this can lead to undefined behavior.

.. _ug_radio_mpsl_cx_nrf700x:

nRF70 Series Wi-Fi coexistence
==============================

Refer to :ref:`ug_radio_coex_mpsl_cx_based` for the general requirements of this implementation.

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

#. On the nRF5340, apply the same devicetree node mentioned in Step 1 to the network core.
   Apply the overlay to the correct network-core child image by creating an overlay file named :file:`child_image/*childImageName*.overlay` in your application directory, for example :file:`child_image/multiprotocol_rpmsg.overlay`.

   The ``*childImageName*`` string must assume one of the following values:

   *  ``multiprotocol_rpmsg`` for multiprotocol applications having support for both 802.15.4 and Bluetooth.
   *  ``802154_rpmsg`` for applications having support for 802.15.4, but not for Bluetooth.
   *  ``hci_ipc`` for application having support for Bluetooth, but not for 802.15.4.

#. Enable the following Kconfig options:

   * :kconfig:option:`CONFIG_MPSL_CX`
   * :kconfig:option:`CONFIG_MPSL_CX_NRF700X`

   .. note::
      If a ``nordic,nrf700x-coex`` compatible node is present in the devicetree and :kconfig:option:`CONFIG_MPSL_CX` is set to ``y``, :kconfig:option:`MPSL_CX_NRF700X` will be selected by default.

.. _ug_radio_mpsl_cx_generic_3wire:

Generic three wire coexistence
==============================

Refer to :ref:`ug_radio_coex_mpsl_cx_based` for the general requirements of this implementation.

Hardware description
--------------------

The generic three wire interface consists of the signals listed in the table below.
The *Pin* is a generic pin name of a PTA, identified rather by its function.
The *Direction* is from the point of view of the SoC running the SR protocol.
The *DT property* is the name of the devicetree node property that configures the connection between the SoC running SR protocol and the nRF70 Series device.

.. table:: Generic three wire coexistence protocol pins

   ============  =========  =================================  ==============
   Pin           Direction  Description                        DT property
   ============  =========  =================================  ==============
   REQUEST       Out        Request signal                     req-gpios
   PRIORITY      Out        SR transaction direction TX or RX  pri-dir-gpios
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

#. On the nRF5340, apply the same devicetree node mentioned in Step 1 to the network core.
   Apply the overlay to the correct network-core child image by creating an overlay file named :file:`child_image/*childImageName*.overlay` in your application directory, for example :file:`child_image/multiprotocol_rpmsg.overlay`.

   The ``*childImageName*`` string must assume one of the following values:

   * ``multiprotocol_rpmsg`` for multiprotocol applications having support for both 802.15.4 and Bluetooth.
   * ``802154_rpmsg`` for applications having support for 802.15.4, but not for Bluetooth.
   * ``hci_ipc`` for application having support for Bluetooth, but not for 802.15.4.

#. Enable the following Kconfig options:

   * :kconfig:option:`CONFIG_MPSL_CX`
   * :kconfig:option:`CONFIG_MPSL_CX_THREAD`


.. _ug_radio_mpsl_cx_custom:

Custom coexistence implementations
=======================================

Refer to :ref:`ug_radio_coex_mpsl_cx_based` for the general requirements that must be fulfilled by the implementation.

To add a custom coexistence implementation based on the MPSL CX API, complete following steps:

1. Determine the hardware interface of your PTA.
   If your PTA uses an interface different from the ones already provided by the |NCS|, you need to provide a devicetree binding file.
   See :file:`nrf/dts/bindings/radio_coex/generic-radio-coex-three-wire.yaml` file for an example.
#. Extend the Kconfig choice :kconfig:option:`CONFIG_MPSL_CX_CHOICE` with a Kconfig option allowing to select the new coex implementation.
#. Write the implementation for your PTA.
   See the :file:`nrf/subsys/mpsl/cx/thread/mpsl_cx_thread.c` file for an example.
   Add the C source file(s) with the implementation, which must contain the following parts:

   * The implementation of the functions required by the interface structure :c:struct:`mpsl_cx_interface_t`.
     Refer to :ref:`MPSL CX API <mpsl_api_sr_cx>` for details.
   * The initialization code initializing the required hardware resources, based on devicetree information.
   * A call to the function :c:func:`mpsl_cx_interface_set()` during the system initialization.

#. Add the necessary CMakeLists.txt entries to get your code compiled when the new Kconfig choice option you added is selected.

.. _ug_radio_coex_bluetooth_only_1wire:

Bluetooth-only 1-wire coexistence
=================================

Refer to :ref:`ug_radio_coex_bluetooth_only_based` for the general requirements of this implementation.

The Bluetooth-only 1-wire coexistence feature allows the :ref:`SoftDevice Controller <nrfxlib:softdevice_controller>` to coexist alongside an LTE device on a separate chip.
It is specifically designed for the coex interface of the nRF91 Series SiP.
The implementation is based on :ref:`nrfxlib:mpsl_bluetooth_coex_1wire`, which is provided into the :ref:`nrfxlib:mpsl` (MPSL) library.


Enabling Bluetooth-only 1-wire coexistence
------------------------------------------

Enable the following Kconfig options:

   * :kconfig:option:`CONFIG_MPSL_CX`
   * :kconfig:option:`CONFIG_MPSL_CX_BT_1WIRE`

The configuration is set using devicetree (DTS).
For more information about devicetree overlays, see :ref:`zephyr:use-dt-overlays`.
See :file:`samples/bluetooth/radio_coex_1wire/boards/nrf52840dk_nrf52840.overlay` for an example of a devicetree overlay.
The elements are described in the bindings: :file:`dts/bindings/radio_coex/sdc-radio-coex-one-wire.yaml`.
See :file:`samples/bluetooth/radio_coex_1wire` for a sample application using 1-wire coexistence.
