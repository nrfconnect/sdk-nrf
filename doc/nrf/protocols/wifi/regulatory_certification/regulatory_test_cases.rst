.. _ug_wifi_regulatory_test_cases:

Regulatory test cases
#####################

This section provides an overview of required regulatory test cases and describes where to find the recommended command sets for the test applications.

The following references are CE regulatory compliance documents:

* 2.4 GHz band RF compliance: EN 300 328 V2.2.2
* 5 GHz band RF compliance: EN 301 893 V2.1.1
* :term:`Electromagnetic Compatibility (EMC)` tests:

  * EN 301 489-1 V2.2.3
  * EN 301 489-17 V3.2.3

The following table lists general RF compliance tests, recommendations for test samples, and links to the related sections in this document that explains how to use the samples for the nRF70 Series devices.

+---------------------------------------------+----------------------------+---------------------------------------------------------------+
| Test case                                   | Test samples               | Reference sections                                            |
+=============================================+============================+===============================================================+
| Center frequencies                          | Wi-Fi® Radio test (TX)     | :ref:`ug_wifi_radio_sample_for_transmit_tests`                |
+---------------------------------------------+----------------------------+                                                               +
| Occupied Channel Bandwidth                  | Wi-Fi Radio test (TX)      |                                                               |
+---------------------------------------------+----------------------------+                                                               +
| Power, Power Density                        | Wi-Fi Radio test (TX)      |                                                               |
+---------------------------------------------+----------------------------+                                                               +
| Transmitter unwanted emissions (Out of Band | Wi-Fi Radio test (TX)      |                                                               |
| (OOB) and spurious domains)                 |                            |                                                               |
+---------------------------------------------+----------------------------+---------------------------------------------------------------+
| Receiver spurious emissions                 | Wi-Fi Radio test (RX/PER)  | :ref:`ug_wifi_radio_test_for_per_measurements`                |
+---------------------------------------------+----------------------------+                                                               +
| Receiver Blocking                           | Wi-Fi Radio test (RX/PER)  |                                                               |
+---------------------------------------------+----------------------------+---------------------------------------------------------------+
| Adaptivity                                  | Wi-Fi Radio test (TX)      | :ref:`ug_wifi_radio_sample_for_transmit_tests`,               |
|                                             |                            | :ref:`ug_using_wifi_shell_sample`, and                        |
|                                             |                            | :ref:`ug_wifi_adaptivity_test_procedure`                      |
+---------------------------------------------+----------------------------+---------------------------------------------------------------+
| Dynamic Frequency Selection (DFS)           | Wi-Fi Radio test (TX)      | :ref:`ug_wifi_radio_sample_for_transmit_tests`                |
+---------------------------------------------+----------------------------+                                                               +
| Transmit Power Control (TPC)                | Wi-Fi Radio test (TX)      |                                                               |
+---------------------------------------------+----------------------------+---------------------------------------------------------------+

The following table lists general EMC compliance tests, recommendations for test samples, and links to the related sections in this document that explains how to use the samples for the nRF70 Series.

+---------------------------------+--------------------------+-------------------------------------------------+
| Test case                       | Test samples             | Reference sections                              |
+=================================+==========================+=================================================+
| Emissions                       | Wi-Fi Radio test (TX)    | :ref:`ug_wifi_radio_sample_for_transmit_tests`  |
+---------------------------------+--------------------------+-------------------------------------------------+
| Immunity                        | Wi-Fi Radio test (RX/PER)| :ref:`ug_wifi_radio_test_for_per_measurements`  |
+---------------------------------+--------------------------+-------------------------------------------------+
| :term:`Electrostatic Discharge  | Wi-Fi Station (STA)      | :ref:`ug_using_wifi_station_sample`             |
| (ESD)`                          |                          |                                                 |
+---------------------------------+--------------------------+-------------------------------------------------+
