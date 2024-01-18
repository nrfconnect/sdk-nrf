.. _coap_server_sample:

Thread: CoAP Server
###################

.. contents::
   :local:
   :depth: 2

The :ref:`Thread <ug_thread>` CoAP Server sample demonstrates controlling light resources within an OpenThread network.
This sample exposes resources in the network.
To access them, you need another sample that is compatible with the OpenThread network.
The recommended sample referenced on this page is :ref:`coap_client_sample`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use one or more of these development kits as the Thread CoAP Server.
You also need one or more compatible development kits programmed with the :ref:`coap_client_sample` sample.

.. include:: /includes/tfm.txt

Overview
********

This sample demonstrates how to expose resources that can be accessed by other devices in the same Thread network.
You can use this sample application as a starting point to implement a :ref:`CoAP <zephyr:coap_sock_interface>` application.

The following CoAP resources are exposed on the network by this sample:

* ``/light`` - used to control **LED 4**
* ``/provisioning`` - used to perform provisioning

This sample uses the native `OpenThread CoAP API`_ for communication.
For new application development, use :ref:`Zephyr's CoAP API<zephyr:coap_sock_interface>`.
For example usage of the Zephyr CoAP API, see the :ref:`coap_client_sample` sample.

User interface
**************

Button 4:
  Pressing results in entering the pairing mode for a limited period of time.

LED 1:
  Lit when the OpenThread connection is established.

LED 3:
  Blinks when the pairing mode is enabled.

LED 4:
  Turned on and off by messages sent from the client nodes.

Configuration
*************

|config|

.. _coap_server_sample_activating_variants:

Snippets
========

.. include:: /includes/sample_snippets.txt

The following snippets are available:

* ``debug`` - Enables debugging the Thread sample by enabling :c:func:`__ASSERT()` statements globally.
* ``logging`` - Enables logging using RTT.
  For additional options, refer to :ref:`RTT logging <ug_logging_backends_rtt>`.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Building and running
********************
.. |sample path| replace:: :file:`samples/openthread/coap_server`

|enable_thread_before_testing|

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test it:

#. Program at least one development kit with the :ref:`coap_client_sample` sample and reset it.
#. Turn on the Simple CoAP Client node.
   This node becomes the Thread network Leader.
#. Turn on all the other nodes, including the Simple CoAP Server nodes.
   They enter the network as Children, and gradually become Routers.

   .. note::
      It can take up to 15 seconds for Thread to establish the network.

#. Press **Button 2** on the client node to control **LED 4** on all server nodes.
#. To pair a client with a server, complete the following steps:

   a. Press **Button 4** on a server node to enable pairing.
   #. Press **Button 3** on a client node to pair it with the server node in the pairing mode.

#. Press **Button 1** on the client node to control the **LED 4** on the paired server node.

Running OpenThread CLI commands
-------------------------------

You can connect to any of the Simple CoAP Server or Simple CoAP Client nodes with a terminal emulator |ANSI| (for example, nRF Connect Serial Terminal).
See :ref:`test_and_optimize` for the required settings and steps.

Once the serial connection is ready, you can run OpenThread CLI commands.
For complete CLI documentation, refer to `OpenThread CLI Reference`_.

.. note::
    In Zephyr shell, every OpenThread command needs to be preceded with the `ot` keyword.
    For example, ``ot channel 20``.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

OpenThread CoAP API is used in this sample:

* `OpenThread CoAP API`_

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
