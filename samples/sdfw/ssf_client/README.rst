.. _sdfw_ssf_client_sample:

SDFW: Service Framework Client
##############################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how Application and Radio core can request Secure Domain services.
It uses the SDFW Service Framework (SSF).

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp, nrf54h20dk_nrf54h20_cpurad

Overview
********

The sample shows how to use Secure Domain services by invoking some of the available services.
Available Secure Domain Services can be found in :file:`nrf/include/sdfw/sdfw_services`.
The service source files are located in :file:`nrf/subsys/sdfw_services/services`.



Building and running
********************

.. |sample path| replace:: :file:`samples/sdfw/ssf_client`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. In the terminal, observe the responses to the different requests.

Dependencies
************

The sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_rpc`

It uses the following Zephyr libraries:

* :ref:`zephyr:ipc_service_backend_icmsg`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`

The sample also uses drivers from `nrfx`_.
