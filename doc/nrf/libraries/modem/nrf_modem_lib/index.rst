.. _nrf_modem:

Modem library
#############

The Modem library is the primary interface for operating the nRF91 Series modem, establishing the LTE-M and NB-IoT connections, and receiving the position data (GPS).
The library aims to be compatible with the POSIX socket APIs to simplify writing and porting of existing networking applications.

To get started with socket programming, you can check out some introductions and tutorials, such as `Beej's Guide to Network Programming`_.

The Modem library is portable, you can use it with any RTOS.
For more information, see :ref:`nrf_modem_ug_porting`.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   architecture
   supported_features
   sockets
   at_interface
   gnss_interface
   bootloader
   delta_dfu
   softsim_interface
   fault_handling
   modem_trace
   logging
   ug_nrf_modem_porting_os
   limitations
   CHANGELOG
   api
