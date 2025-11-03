.. _ug_cellular_ncs:

Getting started with cellular products
######################################

.. contents::
   :local:
   :depth: 2

For getting started with the cellular products, you can refer to the :ref:`ug_nrf91` documentation, which guides through the features, getting started steps, and advanced usage of the boards.
For a self-paced, hands-on course, enroll in DevAcademy's `Cellular IoT Fundamentals course`_, which describes the LTE-M and NB-IoT technologies and the software architecture of a cellular IoT application based on Nordic Semiconductor nRF91 Series devices.

The :ref:`nrf9160_ug_network_mode` section provides information about switching between LTE-M and NB-IoT.

Cellular IoT in |NCS|
*********************

The following sections explain the different applications, libraries, and samples that use the cellular IoT products.

Modem library
=============

.. include:: ../../app_dev/device_guides/nrf91/nrf91_features.rst
    :start-after: nrf91_modem_lib_start
    :end-before: nrf91_modem_lib_end

Library support
===============

The following |NCS| libraries are used for cellular IoT:

* :ref:`lib_modem`
* :ref:`lib_networking`
* :ref:`liblwm2m_carrier_readme` library
* :ref:`mod_memfault` library

Applications and samples
========================

The following application uses the cellular IoT in |NCS|:

* :ref:`connectivity_bridge`

The following samples are used for the cellular IoT development in |NCS|:

* :ref:`cellular_samples`.
* :ref:`memfault_sample` sample.
* :ref:`networking_samples` - These samples use an nRF91 Series device over LTE or an nRF70 Series device over Wi-Fi® for communication and connection.

Integrations
============

The following integrations are available for cellular IoT in |NCS|:

* :ref:`nRF Cloud <ug_nrf_cloud>` - `nRF Cloud`_ is `Nordic Semiconductor's IoT cloud platform`_ that allows you to remotely manage and update your IoT devices using :term:`Firmware Over-the-Air (FOTA) <Firmware Over-the-Air (FOTA) update>`.
* :ref:`Memfault <ug_memfault>` - A cloud-based web application with |NCS| compatibility that monitors devices and allows you to debug issues remotely through an LTE or Wi-Fi network.
* :ref:`AVSystem <ug_avsystem>` - Software solution provider that provides a device management platform.

Power optimization
==================

For optimizing the power consumption of your cellular application, you can use the `Online Power Profiler (OPP)`_ tool.
See :ref:`app_power_opt_nRF91` for more information.
