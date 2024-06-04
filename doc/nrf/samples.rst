.. _samples:

Samples
#######

The |NCS| provides samples that specifically target Nordic Semiconductor devices and show how to implement typical use cases with Nordic Semiconductor libraries and drivers.

Samples showcase a single feature or library, while :ref:`applications` include a variety of libraries to implement a specific use case.

Zephyr also provides a variety of :ref:`zephyr:samples-and-demos`, including very simple :ref:`zephyr:basic-sample`.
These samples are a good starting point for understanding how to put together your own application.
However, Zephyr samples and applications are not tested and verified to work with the |NCS| releases.

General information about samples in the |NCS|
   * |ncs_unchanged_samples_note|
   * |ncs_oot_sample_note|
   * All samples in the |NCS| use :ref:`lib_fatal_error` library and are configured to perform a system reset if a fatal error occurs.
     This behavior is different from how fatal errors are handled in the Zephyr samples.
     You can change the default behavior by updating the configuration option :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR`.
   * All samples in the |NCS| are tested and verified in accordance with their :ref:`maturity level <software_maturity>`.

.. note::
   |common_sample_components_desc|

If you want to list samples available for one or more specific boards, `use the nRF Connect for Visual Studio Code extension to filter them <Browse samples_>`_.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   samples/amazon_sidewalk
   samples/bl
   samples/fast_pair
   samples/mesh
   samples/cellular
   samples/crypto
   samples/debug
   samples/dect
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
   samples/sdfw
   samples/sensor
   samples/suit
   samples/tfm
   samples/thread
   samples/zigbee
   samples/wifi
   samples/other
