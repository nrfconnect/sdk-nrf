.. _samples:

Samples
#######

The |NCS| provides samples that specifically target Nordic Semiconductor devices and show how to implement typical use cases with Nordic Semiconductor libraries and drivers.

Samples showcase a single feature or library, while :ref:`applications` include a variety of libraries to implement a specific use case.

Zephyr also provides a variety of :zephyr:code-sample-category:`samples`, including very simple :zephyr:code-sample-category:`basic`.
These samples are a good starting point for understanding how to put together your own application.
However, Zephyr samples and applications are not tested and verified to work with the |NCS| releases.

.. |sample_test| replace:: samples

.. |sample_test_file| replace:: :file:`nrf/samples/zephyr`

.. |zephyr_sample_test_file| replace:: :file:`zephyr/samples`

.. include:: /includes/zephyr_sample_test.txt

.. samples_general_info_start

General information about samples in the |NCS|
   * |ncs_unchanged_samples_note|
   * |ncs_oot_sample_note|
   * |samples_tfm_info|
   * All samples in the |NCS| use :ref:`lib_fatal_error` library and are configured to perform a system reset if a fatal error occurs.
     This behavior is different from how fatal errors are handled in the Zephyr samples.
     You can change the default behavior by updating the configuration option :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR`.
   * All samples in the |NCS| are tested and verified in accordance with their :ref:`maturity level <software_maturity>`.

.. samples_general_info_end

.. note::
   |common_sample_components_desc|

|filter_samples_by_board|

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   samples/amazon_sidewalk
   samples/fast_pair
   samples/mesh
   samples/bl
   samples/cellular
   samples/crypto
   samples/ironside_se
   samples/debug
   samples/dect
   samples/dfu
   samples/edge
   samples/esb
   samples/gazell
   samples/keys
   samples/matter
   samples/net
   samples/nfc
   samples/nrf5340
   samples/peripheral
   samples/pmic
   samples/serialization
   samples/sensor
   samples/thread
   samples/tfm
   samples/wifi
   samples/zigbee
   samples/other
