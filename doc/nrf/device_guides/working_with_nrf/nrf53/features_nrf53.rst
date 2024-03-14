.. _features_nrf53:

Features of nRF53 Series
########################

.. contents::
   :local:
   :depth: 2

The nRF53 Series System-on-Chips (SoC) integrate a high-performance Arm® Cortex®-M33 dual-core processor with a 2.4 GHz RF transceiver, in addition to advanced security features.
All nRF53 Series SoC support Bluetooth® 5.1 and Bluetooth Mesh, in addition to multiprotocol capabilities.

For additional information, refer to the following resources:

* `nRF53 Series`_ for the technical documentation on the nRF53 Series chips and associated kits.
* :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.

For the nRF5340 DK, see the following user guides:

* :ref:`ug_nrf5340_gs` for getting started with the nRF5340 DK.
* :ref:`ug_nrf5340` for advanced topics specific to the nRF5340 DK.

For the Nordic Thingy:53, see the following user guides:

* :ref:`ug_thingy53_gs` for getting started with the Thingy:53.
* :ref:`ug_thingy53` for advanced topics specific to the Thingy:53.

Dual-core architecture
**********************

The nRF53 Series introduces a dual-core Arm Cortex-M33 architecture, consisting of the following cores:

* An application core for high-performance application execution.
* A dedicated network core optimized for wireless communication efficiency.

The dual-core architecture allows for concurrent execution of application logic and wireless communication tasks.
This ensures that the application processes do not interfere with network operations, enabling IoT with higher responsiveness and reliability.
With one core dedicated to application tasks and the other to connectivity, devices can benefit from optimized performance in both areas.

The separation of application and network functionalities simplifies firmware updates, allowing modifications on one processor without affecting the other.

This architecture is ideal for devices requiring significant data processing alongside continuous wireless connectivity.

Inter-core communication
========================

Interprocessor Communication (IPC) allows for inter-core communication between the two cores.
Communication between the application core and the network core happens through a shared memory area.
The application core memory is mapped to the network core memory map.
This means that the network core can access and use the application core memory for shared memory communication.
IPC is used to indicate to the other core that there is new data available to pick up.
The actual data exchange is handled by Open Asymmetric Multi-Processing (OpenAMP).

Enhanced security features
**************************

In addition to a :ref:`secure bootloader chain <ug_bootloader>` (as in the nRF52 Series), the nRF53 Series incorporates the following within its dual-core architecture:

* Arm TrustZone® for hardware-enforced isolation.
* CryptoCell-312 for encryption, decryption, and cryptographic operations.

These elements provide a platform for secure execution environments and secure data handling.
In addition, the compartmentalization provided by the dual-core setup enables enhanced security measures, with critical operations and sensitive data isolated from general application processes.
Utilizing Arm TrustZone technology on the application processor can further secure devices against tampering and cyber threats.

Supported protocols
*******************

The nRF53 Series supports several protocols, including the following protocols:

* Bluetooth Low Energy
* Thread and Zigbee (IEEE 802.15.4)

See :ref:`ug_nrf5340_protocols` for more information on supported protocols and use cases for the nRF5340 DK in particular.

Multiprotocol support
*********************

The nRF53 Series supports simultaneous multiprotocol operation with Thread and Zigbee (IEEE 802.15.4), enabled by the dual-core setup.

See :ref:`ug_nrf5340_protocols_multiprotocol` for more information on multiprotocol support for the nRF5340 DK in particular.
