.. _nrf5340_empty_net_core:

nRF5340: Empty firmware for network core
########################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to generate an empty network core firmware.
When building an MCUboot child image with the nRF5340 :ref:`ug_nrf5340_multi_image_dfu` feature, the empty network core sample is built together with the application to prevent build failures related to missing Partition Manager definitions in the :ref:`ug_multi_image`.
The sample is used only by the applications that do not use the network core.
In the mentioned case, the empty network core sample is automatically added to build by the :kconfig:option:`CONFIG_NCS_SAMPLE_EMPTY_NET_CORE_CHILD_IMAGE` option which depends on the :kconfig:option:`CONFIG_SOC_NRF53_CPUNET_ENABLE` option.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf5340/empty_net_core`

.. include:: /includes/build_and_run.txt

Testing
=======

The sample does not build firmware for the application core and because of that the sample cannot be tested separately.
