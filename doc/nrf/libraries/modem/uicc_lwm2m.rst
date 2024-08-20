.. _lib_uicc_lwm2m:

UICC LwM2M
##########

.. contents::
   :local:
   :depth: 2

The UICC LwM2M library provides functionality to read LwM2M bootstrap configuration from SIM.

Configuration
*************

To enable the UICC LwM2M library, configure the :kconfig:option:`CONFIG_UICC_LWM2M` Kconfig option.

Dependencies
************

The UICC LwM2M library requires the :ref:`nrfxlib:nrf_modem` to use AT commands.

API documentation
*****************

| Header file: :file:`include/modem/uicc_lwm2m.h`
| Source file: :file:`lib/uicc_lwm2m/uicc_lwm2m.c`

.. doxygengroup:: uicc_lwm2m
