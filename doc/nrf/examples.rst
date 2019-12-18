.. _examples:

Samples and applications
########################

The `Zephyr Project`_ contains a variety of application samples and demos.
Documentation for those is available in the :ref:`zephyr:samples-and-demos` section.

Nordic Semiconductor provides additional examples that specifically target Nordic Semiconductor devices and show how to implement typical use cases with our libraries and drivers.
Samples showcase a single feature or library, while applications include a variety of libraries to implement a specific use case.

.. note::
   All samples in the |NCS| are configured to perform a system reset if a fatal error occurs.
   This behavior is different from how fatal errors are handled in the Zephyr samples.
   You can change the default behavior by updating the configuration option :option:`CONFIG_RESET_ON_FATAL_ERROR`.

.. toctree::
   :maxdepth: 2

   samples
   applications
