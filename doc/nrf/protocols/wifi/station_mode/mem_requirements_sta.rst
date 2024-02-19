.. _ug_wifi_mem_req_sta_mode:

Memory requirements for Wi-Fi applications in Station mode
##########################################################

Code and RAM memory footprint requirements differ depending on the selected platform and the application example.

The following tables list memory requirement values for selected Wi-Fi® samples supporting Station mode.

Footprint values are provided in kilobytes (KB).

.. tabs::

   .. tab:: nRF52840 DK

      The following table lists memory requirements for sample running on the :ref:`nRF52840 DK <programming_board_names>` (:ref:`nrf52840dk_nrf52840 <zephyr:nrf52840dk_nrf52840>`).

      +--------------------------------------+-------------+-------------------------------------------+----------------------+---------------------------------+--------------------+----------------------+
      | Sample                               |   Total ROM |   Wi-Fi driver ROM (incl. nRF70 FW patch) |   WPA supplicant ROM |   Total RAM (incl. static HEAP) |   Wi-Fi driver RAM |   WPA supplicant RAM |
      +======================================+=============+===========================================+======================+=================================+====================+======================+
      | :ref:`Station <wifi_station_sample>` |         499 |                                        98 |                  220 |                             224 |                164 |                   13 |
      +--------------------------------------+-------------+-------------------------------------------+----------------------+---------------------------------+--------------------+----------------------+

   .. tab:: nRF7002 DK

      The following table lists memory requirements for samples running on the :ref:`nRF7002 DK <programming_board_names>` (:ref:`nrf7002dk_nrf5340_cpuapp <nrf7002dk_nrf5340>`).

      +--------------------------------------+-------------+-------------------------------------------+----------------------+---------------------------------+--------------------+----------------------+
      | Sample                               |   Total ROM |   Wi-Fi driver ROM (incl. nRF70 FW patch) |   WPA supplicant ROM |   Total RAM (incl. static HEAP) |   Wi-Fi driver RAM |   WPA supplicant RAM |
      +======================================+=============+===========================================+======================+=================================+====================+======================+
      | :ref:`MQTT <mqtt_sample>`            |         655 |                                       100 |                  259 |                             384 |                171 |                   13 |
      +--------------------------------------+-------------+-------------------------------------------+----------------------+---------------------------------+--------------------+----------------------+
      | :ref:`Station <wifi_station_sample>` |         497 |                                       100 |                  220 |                             222 |                164 |                   13 |
      +--------------------------------------+-------------+-------------------------------------------+----------------------+---------------------------------+--------------------+----------------------+
