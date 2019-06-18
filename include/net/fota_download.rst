.. _lib_fota_download:

FOTA Download
#############

The firmware over-the-air (FOTA) download library provides functions to download a firmware file as an upgrade candidate to the secondary slot of MCUboot.

This is done using the :ref:`lib_download_client` and ``flash_img`` libraries.

Once the download has been started, all received data fragments are passed to the ``flash_img`` library, which stores them in the appropriate memory address for an firmware upgrade candidate.

When the download client sends the event indicating that the download has completed, the last data fragments are flushed to persistent memory, and the received firmware is tagged as an upgrade candidate.
Lastly the download client is told to disconnect from the server.


API documentation
*****************

| Header file: :file:`include/fota_download.h`
| Source files: :file:`subsys/net/lib/fota_download/src/`

.. doxygengroup:: fota_download
   :project: nrf
   :members:
