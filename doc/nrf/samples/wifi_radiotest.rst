.. _wifi_radiotest_samples:

Wi-Fi: Radio test samples
###########################

The following samples demonstrate how to use the Wi-Fi® radio test commands.
The samples are available in two configurations:

* Single domain - The Wi-Fi radio test and Bluetooth® LE radio test run on the same core (application core).
  This configuration generates a single image that can be flashed to the device.
* Multi domain - The Wi-Fi radio test runs on the application core, and the Bluetooth LE radio test runs on the network core.
  This configuration generates two images, one for each core, which can be flashed to the device.

The choice of configuration depends on the platform, whether the Bluetooth LE radio test is required, and whether the Bluetooth LE stack runs on the application core or the network core.

The exact definition of domain or core depends on the hardware platform.
For example, the nRF5340 SoC has two cores, the application core and the network core, whereas the nRF54L15 SoC has a single core.


.. toctree::
   :maxdepth: 1
   :caption: Subpages
   :glob:

   ../../../samples/wifi/radio_test/*/README
