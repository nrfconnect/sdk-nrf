.. _lte_lc_readme:

LTE link controller
###################

.. contents::
   :local:
   :depth: 2

The LTE link controller library is intended to be used for simpler control of LTE connections on an nRF9160 SiP.

This library provides the API and configurations for setting up an LTE connection with definite properties.
Some of the properties that can be configured are Access Point Name (APN), support for LTE modes (NB-IoT or LTE-M), support for GPS, Power Saving Mode (PSM), and extended Discontinuous Reception (eDRX) parameters.
Also, the modem can be locked to use specific LTE bands by modifying the properties.

The link controller is enabled with the :option:`CONFIG_LTE_LINK_CONTROL` option.

Connection to an LTE network can take some time.
The connection time is mostly in the range of few seconds and can go up to a few minutes.
The LTE link controller library provides both blocking and non-blocking methods of connection.
This provides the user an ability to decide the best way to handle the waiting time.

An event handler can be registered with the LTE link controller library using one of the following structures:

* :c:type:`lte_lc_register_handler`
* :c:type:`lte_lc_connect_async`
* :c:type:`lte_lc_init_and_connect_async`

You can thus set the event handler separately or as part of a call to a non-blocking function to establish an LTE connection.

The handler is called with events as parameters when connection parameters change.
Examples of connection parameters are listed below.

* Network registration status
* PSM parameters
* eDRX parameters
* RRC mode
* Charge of currently connected LTE cell

API documentation
*****************

| Header file: :file:`include/modem/lte_lc.h`
| Source file: :file:`lib/lte_link_control/lte_lc.c`

.. doxygengroup:: lte_lc
   :project: nrf
   :members:
