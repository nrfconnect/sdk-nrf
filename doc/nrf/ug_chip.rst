.. _ug_chip:

Connected Home over IP (CHIP)
#############################

.. chip_intro_start

`Connected Home over IP`_ is an open-source application layer project that aims at creating a unified communication standard across smart home devices, mobile applications, and cloud services.
It supports a wide range of existing technologies, including Wi-Fi, Thread, and Bluetooth LE, and uses IPv6-based transport protocols like TCP and UDP to ensure connectivity between different kinds of networks.

CHIP is in an early development stage and must be treated as an experimental feature.

.. chip_intro_end

The |NCS| allows you to develop applications that are compatible with the Connected Home over IP project.

Architecture
************

CHIP defines an application layer on top of the IPv6-based transport protocols.
This allows for routing messages regardless of the underlying physical and link layers.

.. figure:: images/CHIP_IP_pyramid.png
   :alt: CHIP architecture overview

   CHIP architecture overview

The CHIP application layer can be broken down into several main components, from IP framing and transport management up to Data Model structure and Application itself.
For detailed description, see the `CHIP Protocol Overview`_ page.

Integration with |NCS|
======================

The CHIP project is included in the |NCS| as one of the submodule repositories managed with the :ref:`zephyr:west` tool.
That is, the code used for |NCS| and CHIP integration is stored in the CHIP repository (nRF Connect platform) and is compiled when building one of the available :ref:`chip_samples`.
Both instances depend on each other, but their development is independent to ensure that they both support the latest stable version of one another.

CHIP is located on the top application layer of the integration model, looking from the networking point of view.
|NCS| and Zephyr provide the Bluetooth LE and Thread stacks, which must be integrated with the CHIP stack using a special intermediate layer.
The |NCS|'s Multiprotocol Service Layer (MPSL) driver allows running Bluetooth LE and Thread concurrently on the same radio chip.

.. figure:: images/chip_nrfconnect_overview_simplified_ncs.svg
   :alt: nRF Connect platform in CHIP

   nRF Connect platform in CHIP

For detailed description, see the `nRF Connect platform overview`_ CHIP documentation page.

Configuration in |NCS|
**********************

The CHIP nRF Connect platform is built when programming any of the available :ref:`chip_samples` to the supported development kits, which creates a CHIP accessory device.
Such a device requires additional configuration if it is to be paired and controlled remotely over a CHIP network built on top of a low-power, 802.15.4 Thread network.
The following figure shows an overview of the network topology with the required configuration elements.

.. figure:: images/CHIP_configuration_overview.jpg
   :scale: 50 %
   :alt: CHIP network topology example with Thread devices

   CHIP network topology example with Thread devices

For information about how to configure the required components, see the following documentation:

* For configuring Wi-Fi Access Point and Thread Border Router:

  * `Building and programming OpenThread RCP firmware`_ - This will create the OpenThread RCP device required for forming the Thread network.
  * `Configuring PC as Thread Border Router`_

* For setting up CHIP Controller and Bluetooth LE Commissioning: `Building and installing Android CHIPTool`_
* For preparing the accessory device and commissioning it: `Preparing and commissioning accessory device`_

You can find this information in the CHIP sample documentation as well.
If you are new to CHIP, check also the tutorials on `DevZone`_.

Samples
*******

See :ref:`chip_samples` for the list of available CHIP samples.
