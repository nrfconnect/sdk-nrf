.. _lib_fota_download:

Firmware over-the-air download
##############################

The firmware over-the-air (FOTA) download library provides functions to download a firmware file as an update candidate to the secondary slot of MCUboot.

Both :ref:`lib_download_client` and Zephyr`s MCUboot libraries are used to achieve this.


API documentation
*****************

| Header file: :file:`include/fota_download.h`
| Source files: :file:`subsys/net/lib/fota_download/src/`

.. doxygengroup:: fota_download
   :project: nrf
   :members:
