.. _supl_client:

SUPL client
###########

The Secure User-Plane Location (SUPL) client module integrates the external SUPL client library into the |NCS|.
This library can be used to receive A-GPS data over the SUPL protocol.

The SUPL client library is released under a different license than the |NCS|.
Therefore, you must download and install it separately to be able to use this module.
Before you download the library, read through the license on the webpage.
You must accept the license before you can download the library.

Downloading and installing
**************************

You can download the SUPL client library from the `nRF9160 DK product page <SUPL client download_>`_.

Download the nRF9160 SiP SUPL client library zip file and extract it into the :file:`nrf/ext/lib/bin/supl/` folder.
Make sure to maintain the folder structure that is used in the zip file.

Configuration
*************

To enable the SUPL client library, set :option:`CONFIG_SUPL_CLIENT_LIB` to ``y``.
See :ref:`configure_application` for information on how to change configuration options.


API documentation
*****************

| Header file: :file:`include/supl_os_client.h`
| Source files: :file:`lib/supl/`

.. doxygengroup:: supl_os
   :project: nrf
   :members:
