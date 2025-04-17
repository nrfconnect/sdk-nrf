.. _ug_wifi_mem_req_softap_mode:

Memory requirements for Wi-Fi applications in SoftAP mode
#########################################################

Code and RAM memory footprint requirements differ depending on the selected platform, networking application layer, and application example.

Footprint values are provided in kilobytes (KB).

.. tabs::

   .. tab:: nRF54H20 DK

      The following table lists the memory requirements for :ref:`SoftAP <wifi_softap_sample>` sample running on the :ref:`nRF54H20 DK <programming_board_names>` (:ref:`nrf54h20dk/nrf54h20/cpuapp <zephyr:nrf54h20dk_nrf54h20>`).

      +---------------------------------------------------------------+-------------+--------------------+----------------------+----------------------+---------------------------------+--------------------+----------------------+
      | Sample                                                        |   Total ROM |   Wi-Fi driver ROM |   nRF70 FW Patch ROM |   WPA supplicant ROM |   Total RAM (incl. static HEAP) |   Wi-Fi driver RAM |   WPA supplicant RAM |
      +===============================================================+=============+====================+======================+======================+=================================+====================+======================+
      | :ref:`SoftAP <wifi_softap_sample>`                            |         693 |                133 |                   79 |                  259 |                             222 |                 79 |                   63 |
      +---------------------------------------------------------------+-------------+--------------------+----------------------+----------------------+---------------------------------+--------------------+----------------------+

   .. tab:: nRF54L15 DK

      The following table lists the memory requirements for :ref:`SoftAP <wifi_softap_sample>` sample running on the :ref:`nRF54L15 DK <programming_board_names>` (:ref:`nrf54l15dk/nrf54l15/cpuapp <zephyr:nrf54l15dk_nrf54l15>`).

      +---------------------------------------------------------------+-------------+--------------------+----------------------+----------------------+---------------------------------+--------------------+----------------------+
      | Sample                                                        |   Total ROM |   Wi-Fi driver ROM |   nRF70 FW Patch ROM |   WPA supplicant ROM |   Total RAM (incl. static HEAP) |   Wi-Fi driver RAM |   WPA supplicant RAM |
      +===============================================================+=============+====================+======================+======================+=================================+====================+======================+
      | :ref:`SoftAP <wifi_softap_sample>`                            |         738 |                134 |                   79 |                  282 |                             180 |                 57 |                   63 |
      +---------------------------------------------------------------+-------------+--------------------+----------------------+----------------------+---------------------------------+--------------------+----------------------+

   .. tab:: nRF7002 DK

      The following table lists the memory requirements for :ref:`SoftAP <wifi_softap_sample>` sample running on the :ref:`nRF7002 DK <programming_board_names>` (:ref:`nrf7002dk/nrf5340/cpuapp <nrf7002dk_nrf5340>`).

      +--------------------------------------+-------------+-------------------------------------------+--------------------------+----------------------+---------------------------------+--------------------+----------------------+
      | Sample                               |   Total ROM |   Wi-Fi driver ROM                        |       nRF70 FW patch ROM |   WPA supplicant ROM |   Total RAM (incl. static heap) |   Wi-Fi driver RAM |   WPA supplicant RAM |
      +======================================+=============+===========================================+==========================+======================+=================================+====================+======================+
      | :ref:`SoftAP <wifi_softap_sample>`   |         728 |                                       135 |                       79 |                  283 |                             212 |                 79 |                   63 |
      +--------------------------------------+-------------+-------------------------------------------+--------------------------+----------------------+---------------------------------+--------------------+----------------------+
