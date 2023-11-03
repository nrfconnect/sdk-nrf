.. _mpsl:

Multiprotocol Service Layer
===========================

The Multiprotocol Service Layer (MPSL) is a set of libraries that provide common services for single and multiprotocol implementations.

The libraries are available as soft-float, softfp-float, and hard-float builds for the nRF52 Series and as a soft-float build for the nRF53 Series.

These libraries provide the following key features:

* Timeslot.
  It allows the application to schedule its own events concurrently with other protocols.
* TX Power control.
  It provides an API for setting the maximum TX power per channel.
* Clock control.
  It provides an API for configuring and controlling the low and high-frequency clock.
* FEM.
  It provides an API for controlling external Front-End Modules.
* SoC Temperature.
  It provides an API for obtaining the temperature measured on the SoC.
* IEEE 802.15.4 External Radio Coexistence.
  It provides an API to control coexistence with IEEE 802.15.4.
* Bluetooth External Radio Coexistence.
  It provides an API to control coexistence with Bluetooth.

To enable MPSL in |NCS| applications, use the :kconfig:option:`CONFIG_MPSL` Kconfig option.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   mpsl
   clock
   timeslot
   tx_power_control
   fem
   cx
   bluetooth_coex
   CHANGELOG
   api
