.. _ug_wifi_mem_req_ble:

Memory requirements for Bluetooth LE based provisioning
#######################################################

Code and RAM memory footprint requirements differ depending on the selected platform and the application example.

Footprint values are provided in kilobytes (KB).

The following table lists memory requirements for the :ref:`Bluetooth LE based provision <ble_wifi_provision>` sample running on the :zephyr:board:`nrf7002dk` (``nrf7002dk/nrf5340/cpuapp``).

+---------------------------------------------------------------+-------------+--------------------+----------------------+----------------------+---------------------------------+--------------------+---------------------------------------+--------------------+----------------------+
| Sample                                                        |   Total ROM |     Total used ROM |     Wi-Fi driver ROM |   nRF70 FW Patch ROM |              WPA supplicant ROM |          Total RAM |    Total used RAM (incl. static HEAP) |   Wi-Fi driver RAM |   WPA supplicant RAM |
+===============================================================+=============+====================+======================+======================+=================================+====================+=======================================+====================+======================+
| :ref:`Bluetooth LE based provision <ble_wifi_provision>`      |        1008 |                649 |                   53 |                   84 |                             163 |                448 |                                   310 |                144 |                   39 |
+---------------------------------------------------------------+-------------+--------------------+----------------------+----------------------+---------------------------------+--------------------+---------------------------------------+--------------------+----------------------+
