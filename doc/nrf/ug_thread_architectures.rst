.. _ug_thread_architectures:

OpenThread architectures
########################

This page describes the OpenThread stack architecture and platform designs that are possible with the OpenThread network stack on Nordic Semiconductor devices in |NCS|.

The designs are described from the least to the most complex, that is from simple applications that consist of a single chip running single or multiple protocols to scenarios in which the nRF SoC acts as a network co-processor when the application is running on a much more powerful host processor.

.. contents::
    :local:
    :depth: 1

.. _openthread_stack_architecture:

OpenThread stack architecture
*****************************

OpenThread's portable nature makes no assumptions about platform features.
OpenThread provides the hooks to use enhanced radio and cryptography features, reducing system requirements, such as memory, code, and compute cycles.
This can be done per platform, while retaining the ability to default to a standard configuration.

.. figure:: images/ot-arch_2x.png
   :alt: OpenThread architecture

   OpenThread architecture

.. _ug_thread_architectures_designs_soc_designs:

System-on-Chip designs
**********************

This single-chip solution has the combined RFIC (the IEEE 802.15.4 in case of Thread) and processor.
OpenThread and the application layer run on the local processor.

.. _thread_architectures_designs_soc_designs_single:

Single-chip, single protocol (SoC)
==================================

In this design, the application layer and OpenThread run on the same processor.
The application uses the OpenThread APIs and IPv6 stack directly.

This is the SoC design most commonly used for applications that do not make heavy computations or are battery powered.

This design has the following advantages:

* Lowest cost.
* Lowest power consumption.
* Lowest complexity.

It also has the following disadvantages:

* For some uses, the nRF528xx MCU can be too slow. For example, when the application would like to do complex data processing.
* The application and the network share Flash and RAM space, which can limit the application functionality.
* Dual-bank DFU or an external flash is needed to update the firmware.

.. figure:: /images/thread_platform_design_soc.svg
   :alt: Thread-only architecture

   Thread-only architecture

This platform design is suitable for the following development kits:

+--------------------------------+-----------+------------------------------------------------+-------------------------------+
|Hardware platforms              |PCA        |Board name                                      |Build target                   |
+================================+===========+================================================+===============================+
|:ref:`nRF52840 DK <ug_nrf52>`   |PCA10056   |:ref:`nrf52840dk_nrf52840 <nrf52840dk_nrf52840>`|``nrf52840dk_nrf52840``        |
+--------------------------------+-----------+------------------------------------------------+-------------------------------+
|:ref:`nRF52833 DK <ug_nrf52>`   |PCA10010   |:ref:`nrf52833dk_nrf52833 <nrf52833dk_nrf52833>`|``nrf52833dk_nrf52833``        |
+--------------------------------+-----------+------------------------------------------------+-------------------------------+
|:ref:`nRF5340 PDK <ug_nrf5340>` |PCA10095   |:ref:`nrf5340pdk_nrf5340 <nrf5340pdk_nrf5340>`  |``nrf5340pdk_nrf5340_cpuapp``  |
|(not yet supported)             |           |                                                |                               |
|                                |           |                                                |``nrf5340pdk_nrf5340_cpuappns``|
+--------------------------------+-----------+------------------------------------------------+-------------------------------+

.. note::
    |5340_not_supported|

.. _thread_architectures_designs_soc_designs_multiprotocol:

Single-chip, multiprotocol (SoC)
================================

With nRF52840 and nRF52833 supporting multiple wireless technologies, including IEEE 802.15.4 and Bluetooth Low Energy (|BLE|), the application layer and OpenThread still run on the same processor.

In this multiprotocol design, the SoC ensures either dynamic or switched Thread and |BLE| connectivity.

This design has the following advantages:

* It leverages the benefits of highly integrated SoC, resulting in the lowest cost and the lowest power consumption.
* It allows to run Thread and |BLE| simultaneously on a single chip, which reduces the overall BOM cost.

It also has the following disadvantages:

* |BLE| activity can degrade the connectivity on Thread if not implemented with efficiency in mind.

.. figure:: /images/thread_platform_design_multi.svg
   :alt: Multiprotocol Thread and |BLE| architecture

   Multiprotocol Thread and |BLE| architecture

This platform design is suitable for the following development kits:

+--------------------------------+-----------+------------------------------------------------+-------------------------------+
|Hardware platforms              |PCA        |Board name                                      |Build target                   |
+================================+===========+================================================+===============================+
|:ref:`nRF52840 DK <ug_nrf52>`   |PCA10056   |:ref:`nrf52840dk_nrf52840 <nrf52840dk_nrf52840>`|``nrf52840dk_nrf52840``        |
+--------------------------------+-----------+------------------------------------------------+-------------------------------+
|:ref:`nRF52833 DK <ug_nrf52>`   |PCA10010   |:ref:`nrf52833dk_nrf52833 <nrf52833dk_nrf52833>`|``nrf52833dk_nrf52833``        |
+--------------------------------+-----------+------------------------------------------------+-------------------------------+
|:ref:`nRF5340 PDK <ug_nrf5340>` |PCA10095   |:ref:`nrf5340pdk_nrf5340 <nrf5340pdk_nrf5340>`  |``nrf5340pdk_nrf5340_cpuapp``  |
|                                |           |                                                |                               |
|                                |           |                                                |``nrf5340pdk_nrf5340_cpuappns``|
+--------------------------------+-----------+------------------------------------------------+-------------------------------+

.. note::
    |5340_not_supported|

.. _thread_architectures_designs_cp:

Co-processor designs
********************

In the co-processor designs, with either network co-processor (NCP) or radio co-processor (RCP), the application layer runs on a host processor and communicates with OpenThread through a serial connection using a standardized host-controller protocol (Spinel).
OpenThread can run on either the radio or the host processor.

.. _thread_architectures_designs_cp_ncp:

Network Co-Processor (NCP)
==========================

The standard NCP design has Thread features on the SoC and runs the application layer on a host processor, which is typically more capable than the OpenThread device, although it has greater power demands.
The host processor communicates with the OpenThread device through a serial interface (typically UART or SPI) over the Spinel protocol.

This design is useful for gateway devices or devices that have other processing demands, like IP cameras and speakers.

This design has the following advantages:

* The higher-power host can sleep, while the lower-power OpenThread device remains active to maintain its place in the Thread network.
* Since the SoC is not tied to the application layer, development and testing of applications is independent of the OpenThread build.
* Only the network stack and a thin application reside on the NCP, which reduces the cost of the chip (RAM and Flash usage may be smaller than in an SoC solution with the application).
* It does not require the support for the dual-bank DFU.
  (Host can just replace the old image with a new one.)

It also has the following disadvantages:

* This is the most expensive option, since it requires the application processor.

.. figure:: /images/thread_platform_design_ncp.svg
   :alt: Network Co-Processor architecture

   Network Co-Processor architecture

.. note::
    |connection_options_limited|

This platform design is suitable for the following development kits:

+--------------------------------+-----------+------------------------------------------------+-------------------------------+
|Hardware platforms              |PCA        |Board name                                      |Build target                   |
+================================+===========+================================================+===============================+
|:ref:`nRF52840 DK <ug_nrf52>`   |PCA10056   |:ref:`nrf52840dk_nrf52840 <nrf52840dk_nrf52840>`|``nrf52840dk_nrf52840``        |
+--------------------------------+-----------+------------------------------------------------+-------------------------------+
|:ref:`nRF52833 DK <ug_nrf52>`   |PCA10010   |:ref:`nrf52833dk_nrf52833 <nrf52833dk_nrf52833>`|``nrf52833dk_nrf52833``        |
+--------------------------------+-----------+------------------------------------------------+-------------------------------+
|:ref:`nRF5340 PDK <ug_nrf5340>` |PCA10095   |:ref:`nrf5340pdk_nrf5340 <nrf5340pdk_nrf5340>`  |``nrf5340pdk_nrf5340_cpuapp``  |
|                                |           |                                                |                               |
|                                |           |                                                |``nrf5340pdk_nrf5340_cpuappns``|
+--------------------------------+-----------+------------------------------------------------+-------------------------------+

.. note::
    |5340_not_supported|

.. _thread_architectures_designs_cp_rcp:

Radio Co-Processor (RCP)
========================

.. warning::
    The RCP architecture is currently not supported in |NCS|.

This is a variant of the NCP design where the core of OpenThread lives on the host processor with only a minimal "controller" on the device with the Thread radio.
The host processor typically does not sleep in this design, in part to ensure reliability of the Thread network.

This design is useful for devices that are less sensitive to power constraints.
For example, the host processor on a video camera is always on to process video.

This design has the following advantages:

* OpenThread can use the resources on the more powerful processor.
* It enables the usage of a co-processor that is less capable in comparison with the NCP solution, which reduces the cost.

It also has the following disadvantages:

* The application processor must be woken up on each received frame, even in case a frame must be forwarded to the neighboring device.
* The RCP solution can be less responsive than NCP solution, due to the fact that each frame or command must be communicated over the serial link with the application processor (host).

.. figure:: /images/thread_platform_design_rcp.svg
   :alt: Radio Co-Processor architecture

   Radio Co-Processor architecture

.. note::
    |connection_options_limited|

.. |connection_options_limited| replace:: Spinel connections through SPI and USB are not currently available.
.. |5340_not_supported| replace:: :ref:`nRF5340 PDK <ug_nrf5340>` is not yet supported by Thread in |NCS|.
