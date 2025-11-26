.. _ug_wifi_mem_req_softap:

Memory requirements for SoftAP based provisioning
#################################################

Code and RAM memory footprint requirements differ depending on the selected platform and the application example.

Footprint values are provided in kilobytes (KB).


The following table lists memory requirements for the :ref:`SoftAP based provision <softap_wifi_provision_sample>` sample running on the  :zephyr:board:`nrf7002dk` (``nrf7002dk/nrf5340/cpuapp``).

+---------------------------------------------------------------+-------------+--------------------+-------------------------+-------------------------+---------------------------------+--------------------+---------------------------------------+-------------------------+---------------------------+
| Sample                                                        |   Total ROM |     Total used ROM |        Wi-Fi driver ROM |      nRF70 FW Patch ROM |              WPA supplicant ROM |          Total RAM |    Total used RAM (incl. static HEAP) |        Wi-Fi driver RAM |        WPA supplicant RAM |
+===============================================================+=============+====================+=========================+=========================+=================================+====================+=======================================+=========================+===========================+
| :ref:`SoftAP based provision <softap_wifi_provision_sample>`  |         848 |                718 | Not supported\ :sup:`1` | Not supported\ :sup:`1` |         Not supported\ :sup:`1` |                408 |                                   213 | Not supported\ :sup:`1` |   Not supported\ :sup:`1` |
+---------------------------------------------------------------+-------------+--------------------+-------------------------+-------------------------+---------------------------------+--------------------+---------------------------------------+-------------------------+---------------------------+

| [1]: Link Time Optimization (LTO) breaks symbol hierarchies in ELF files, which prevents measurement.

.. note::

   The footprint of the networking samples for the Zephyr image is derived from the board target with :ref:`Cortex-M Security Extensions enabled <app_boards_spe_nspe_cpuapp_ns>` (``*/ns`` :ref:`variant <app_boards_names>`).
