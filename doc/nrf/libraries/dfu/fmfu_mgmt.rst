.. _lib_fmfu_mgmt:

Mcumgr-based full modem update
##############################

.. contents::
   :local:
   :depth: 2

The full modem update management library allows performing an update over UART using the SMP protocol, providing an API that exposes functions to register command hooks into the MCU manager (mcumgr) management protocol.

It is used by the :ref:`fmfu_smp_svr_sample` sample to provide the following functionalities:

* Modem firmware update to the modem.
* Memory hash function for write verification.

It uses command value ``0`` for getting the hash of an address range and command value ``1`` for uploading firmware data.

Before calling the :c:func:`fmfu_mgmt_init` function, the modem needs to be set into DFU mode.
For more information on how to change the modem mode see :ref:`nrfxlib:nrf_modem`.

API documentation
*****************

| Header file: :file:`include/mgmt/fmfu_mgmt.h` and :file:`include/mgmt/fmfu_mgmt_stat.h`
| Source files: :file:`subsys/mgmt/src/`

.. doxygengroup:: fmfu_mgmt
   :project: nrf
   :members:
