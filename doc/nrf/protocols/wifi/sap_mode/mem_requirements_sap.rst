.. _ug_wifi_mem_req_softap_mode:

Memory requirements for Wi-Fi applications in SoftAP mode
#########################################################

Code and RAM memory footprint requirements differ depending on the selected platform, networking application layer, and application example.

Footprint values are provided in kilobytes (KB).

The following table lists the memory requirements for :ref:`SoftAP <wifi_softap_sample>` sample running on the :ref:`nRF7002 DK <programming_board_names>` (:ref:`nrf7002dk/nrf5340/cpuapp <nrf7002dk_nrf5340>`).

+--------------------------------------+-------------+-------------------------------------------+--------------------------+----------------------+---------------------------------+--------------------+----------------------+
| Sample                               |   Total ROM |   Wi-Fi driver ROM                        |       nRF70 FW patch ROM |   WPA supplicant ROM |   Total RAM (incl. static heap) |   Wi-Fi driver RAM |   WPA supplicant RAM |
+======================================+=============+===========================================+==========================+======================+=================================+====================+======================+
| :ref:`SoftAP <wifi_softap_sample>`   |         682 |                                        59 |                       69 |                  291 |                             309 |                207 |                   13 |
+--------------------------------------+-------------+-------------------------------------------+--------------------------+----------------------+---------------------------------+--------------------+----------------------+
