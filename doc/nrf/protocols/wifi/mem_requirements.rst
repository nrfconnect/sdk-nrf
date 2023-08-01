.. _ug_wifi_mem_req:

Memory requirements for Wi-Fi applications
==========================================

Code and RAM memory footprint requirements differ depending on the selected platform and the application example.

The following tables list memory requirement values for selected Wi-Fi® samples.

Footprint values are provided in kilobytes (KB).

.. tabs::

   .. tab:: nRF52840 DK

      The following table lists memory requirements for samples running on the :ref:`nRF52840 DK <programming_board_names>` (:ref:`nrf52840dk_nrf52840 <zephyr:nrf52840dk_nrf52840>`).

      +----------------------------------------+-------------------+---------------------------------+
      | Sample                                 |   Application ROM |   Total RAM (incl. static HEAP) |
      +========================================+===================+=================================+
      | :ref:`Scan <wifi_scan_sample>`         |               147 |                              64 |
      +----------------------------------------+-------------------+---------------------------------+
      | :ref:`Station <wifi_station_sample>`   |               500 |                             219 |
      +----------------------------------------+-------------------+---------------------------------+

   .. tab:: nRF7002 DK

      The following table lists memory requirements for samples running on the :ref:`nRF7002 DK <programming_board_names>` (:ref:`nrf7002dk_nrf5340_cpuapp <nrf7002dk_nrf5340>`).

      +----------------------------------------+-------------------+---------------------------------+
      | Sample                                 |   Application ROM |   Total RAM (incl. static HEAP) |
      +========================================+===================+=================================+
      | :ref:`MQTT <mqtt_sample>`              |               540 |                             258 |
      +----------------------------------------+-------------------+---------------------------------+
      | :ref:`Scan <wifi_scan_sample>`         |               146 |                              63 |
      +----------------------------------------+-------------------+---------------------------------+
      | :ref:`Station <wifi_station_sample>`   |               498 |                             217 |
      +----------------------------------------+-------------------+---------------------------------+

   .. tab:: nRF9160 DK

      The following table lists memory requirements for samples running on the :ref:`nRF9160 DK <programming_board_names>` (:ref:`nrf9160dk_nrf9160_ns <zephyr:nrf9160dk_nrf9160>`).

      +--------------------------------------------+-------------------+---------------------------------+
      | Sample                                     |   Application ROM |   Total RAM (incl. static HEAP) |
      +============================================+===================+=================================+
      | :ref:`Location <location_sample>`          |               117 |                              64 |
      +--------------------------------------------+-------------------+---------------------------------+
      | :ref:`MQTT <mqtt_sample>`                  |               143 |                              83 |
      +--------------------------------------------+-------------------+---------------------------------+
