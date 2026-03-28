.. _wifi_p2p_sample:

Wi-Fi: P2P
##########

.. contents::
   :local:
   :depth: 2

The P2P sample demonstrates how to establish a Wi-Fi® Peer-to-Peer (P2P) connection between devices using the Wi-Fi Direct® functionality.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********
The sample demonstrates Wi-Fi P2P functionality using the following two operating modes:

* Client (CLI) mode – The device discovers a peer and initiates a P2P connection.
* Group Owner (GO) mode – The device acts as the P2P Group Owner and manages the P2P group.

Wi-Fi P2P enables direct communication between devices without requiring a traditional access point.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/p2p/Kconfig`):

.. options-from-kconfig::
   :show-type:

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/p2p`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk/nrf5340/cpuapp

.. include:: /includes/wifi_refer_sample_yaml_file.txt
