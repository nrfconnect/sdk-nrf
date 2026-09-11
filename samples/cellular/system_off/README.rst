.. _system_off_sample:

.. ncs-sample::
   :title: Cellular: System off

   This samples demonstrates how to use the System OFF functionality on the nRF92 Series.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

This sample can be used as an example of System OFF functionality on the nRF92 Series and for basic power measurement.

The sample supports wakeup from System OFF by GPIO and GRTC.
You can also enable state retention in the System OFF mode.
When state retention is enabled, the sample retains the number of boots, the number of times the system was put in System OFF mode, the total uptime since the initial power-on, and verifies retained data integrity on wakeup.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following Kconfig options:

.. options-from-kconfig::
   :show-type:

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/system_off`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Press button 0 on the development kit to wake up the system.

Sample Output
=============

.. code-block:: console

   *** Booting nRF Connect SDK v3.4.99-8132e53938bb ***
   *** Using Zephyr OS v4.4.99-cc917d7a40ae ***

   System off sample
   Wakeup from System OFF by GPIO
   State retention disabled
   Entering System OFF; press button 0 to restart

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`hwinfo_api`
