.. _nrf5340_empty_net_core:

Empty firmware for multiple core SoCs
#####################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to generate an empty network (or other) core firmware.
The sample is used only by the applications that do not use the other core(s).
An empty image can be added to a project using the following options (depending upon if an SoC has the CPU):

+----------------------------------------------+--------------------------------------------+
| Core                                         | Sysbuild Kconfig                           |
+==============================================+============================================+
| Network/radio processor                      | :kconfig:option:`SB_CONFIG_NETCORE_EMPTY`  |
+----------------------------------------------+--------------------------------------------+
| Fast Lightweight peripheral processor (FLPR) | :kconfig:option:`SB_CONFIG_FLPRCORE_EMPTY` |
+----------------------------------------------+--------------------------------------------+
| Peripheral processor (PPR)                   | :kconfig:option:`SB_CONFIG_PPRCORE_EMPTY`  |
+----------------------------------------------+--------------------------------------------+

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
