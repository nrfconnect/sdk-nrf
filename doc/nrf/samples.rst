.. _samples:

Samples
#######

The |NCS| provides samples that specifically target Nordic Semiconductor devices and show how to implement typical use cases with Nordic Semiconductor libraries and drivers.

Samples showcase a single feature or library, while :ref:`applications` include a variety of libraries to implement a specific use case.

Zephyr also provides a variety of application samples and demos.
Documentation for those is available in Zephyr's :ref:`zephyr:samples-and-demos` section.
For very simple samples, see the :ref:`zephyr:basic-sample`.
Those samples are a good starting point for understanding how to put together your own application.

General information about samples in the |NCS|
   * |ncs_unchanged_samples_note|
   * |ncs_oot_sample_note|
   * All samples in the |NCS| use :ref:`lib_fatal_error` library and are configured to perform a system reset if a fatal error occurs.
     This behavior is different from how fatal errors are handled in the Zephyr samples.
     You can change the default behavior by updating the configuration option :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR`.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   samples/bl
   samples/crypto
   samples/debug
   samples/edge
   samples/gazell
   samples/matter
   samples/multicore
   samples/net
   samples/nfc
   samples/nrf5340
   samples/nrf9160
   samples/pmic
   samples/tfm
   samples/thread
   samples/zigbee
   samples/wifi
   samples/other
