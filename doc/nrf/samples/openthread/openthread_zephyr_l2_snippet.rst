.. _snippet_ot_zephyr_l2:

OpenThread snippet for Zephyr L2 networking layer (ot-zephyr-l2)
=================================================================

.. contents::
   :local:
   :depth: 2

To build with this snippet, follow the instructions in the :ref:`using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=ot-zephyr-l2

Overview
********

This snippet enables the Zephyr networking layer integration for the OpenThread stack.

This architecture uses Zephyr networking layer to communicate OpenThread stack with the IEEE 802.15.4 radio driver.
It integrates Zephyr networking features, such as Sockets, IPv6, DNS, DHCPv6, zperf, and more.

To learn more about this architecture, see :ref:`openthread_stack_architecture`.

.. note::

   This configuration has an impact on the memory and flash usage of your application.
   It also affects the throughput of the network traffic.
