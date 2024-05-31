.. _ug_wifi_mem_req_softap:

Memory requirements for SoftAP based provisioning
#################################################

Code and RAM memory footprint requirements differ depending on the selected platform and the application example.

Footprint values are provided in kilobytes (KB).


The following table lists memory requirements for the :ref:`SoftAP based provision <softap_wifi_provision_sample>` sample running on the :ref:`nRF7002 DK <programming_board_names>` (:ref:`nrf7002dk/nrf5340/cpuapp <nrf7002dk_nrf5340>`).

+-------------------------------------------------------------+-------------+-------------------------------------------+-------------------------------+----------------------+---------------------------------+--------------------+----------------------+
| Sample                                                      |   Total ROM |   Wi-Fi driver ROM                        |            nRF70 FW patch ROM |   WPA supplicant ROM |   Total RAM (incl. static heap) |   Wi-Fi driver RAM |   WPA supplicant RAM |
+=============================================================+=============+===========================================+===============================+======================+=================================+====================+======================+
| :ref:`SoftAP based provision <softap_wifi_provision_sample>`|         666 |                                         0 |                            69 |                    0 |                             302 |                  0 |                    0 |
+-------------------------------------------------------------+-------------+-------------------------------------------+-------------------------------+----------------------+---------------------------------+--------------------+----------------------+

.. note::

   The footprint of the networking samples for the Zephyr image is derived from the board target with :ref:`Cortex-M Security Extensions enabled <app_boards_spe_nspe_cpuapp_ns>` (``*/ns`` :ref:`variant <app_boards_names>`).
