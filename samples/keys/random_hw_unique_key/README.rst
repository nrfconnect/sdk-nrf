.. _random_hw_unique_key:

Random hardware unique key
##########################

.. contents::
   :local:
   :depth: 2

This sample writes random hardware unique keys (HUKs) to the Key Management Unit (KMU), when available, or to the relevant flash memory page, when the KMU is not present.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

When using the :ref:`lib_hw_unique_key` library together with the :ref:`bootloader`, you must provision a hardware unique key for the bootloader into the relevant KMU slot or flash memory page.

To provision the HUKs, build and run this sample before programming the bootloader and application.
It will save the HUKs in the device.

Configuration
*************

|config|

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/keys/random_hw_unique_key`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Observe the following output:

         .. code-block:: console

             Writing random keys to KMU.
             Success!

         If an error occurs, the sample prints a message and raises a kernel panic.

Dependencies
*************

This sample uses the following libraries:

* :ref:`lib_hw_unique_key`
