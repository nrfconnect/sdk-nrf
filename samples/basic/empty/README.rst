.. _nrf5340_empty_net_core:

Empty firmware for multiple core SoCs
#####################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to generate an empty network (or other) core firmware.
The sample is used only by the applications that do not use the other core(s).
In the mentioned case, the empty network core sample is automatically added to build by the :kconfig:option:`SB_CONFIG_NETCORE_EMPTY` sysbuild Kconfig option.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Building and running
********************

.. |sample path| replace:: :file:`samples/basic/empty`

.. include:: /includes/build_and_run.txt

Testing
=======

The sample does not build firmware for the application core and because of that the sample cannot be tested separately.
