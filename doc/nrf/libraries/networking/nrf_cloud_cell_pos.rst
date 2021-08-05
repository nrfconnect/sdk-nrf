.. _lib_nrf_cloud_cell_pos:

nRF Cloud cellular positioning
##############################

.. contents::
   :local:
   :depth: 2

The nRF Cloud cellular positioning library enables applications to request cellular positioning data from `nRF Cloud`_ to be used with the nRF9160 SiP.
This library is an enhancement to the :ref:`lib_nrf_cloud` library.

.. note::
   To use the nRF Cloud cellular positioning service, an nRF Cloud account is needed, and the device needs to be associated with a user's account.

Configuration
*************

Configure the following options to enable or disable the use of this library:

* :kconfig:`CONFIG_NRF_CLOUD`
* :kconfig:`CONFIG_NRF_CLOUD_CELL_POS`


Request and process cellular positioning data
*********************************************

The :c:func:`nrf_cloud_cell_pos_request` function is used to request the cellular location of the device by type.

When nRF Cloud responds with the requested cellular positioning data, the :c:func:`nrf_cloud_cell_pos_process` function processes the received data.
The function parses the data and returns the location information if it is found.

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_cell_pos.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/`

.. doxygengroup:: nrf_cloud_cell_pos
   :project: nrf
   :members:
