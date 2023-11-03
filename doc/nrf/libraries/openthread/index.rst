.. _ot_libs:

OpenThread pre-built libraries
##############################

.. contents::
   :local:
   :depth: 2

The |NCS| provides pre-built OpenThread libraries.
Some of these are certified by the Thread Group for use with Nordic Semiconductor devices.
This simplifies the OpenThread certification process of the final product.
See :ref:`ug_thread_cert` for more information.

The pre-built libraries vary depending on stack configuration and hardware cryptography capabilities:

* The stack configuration depends on the feature set that you use.
  A feature set is a predefined set of configuration options.
  For more information about selecting a feature set and customizing the configuration of the pre-built libraries, see :ref:`nrf:thread_ug_prebuilt`.
* The cryptography variant is selected automatically based on the microcontroller capabilities.

For the full list of configuration options that were used during compilation of the libraries, including the default values, see the :file:`openthread_lib_configuration.txt` file within each library folder.

Certified libraries
*******************

Check the compatibility matrices in the `Infocenter`_ for information about which library variants and versions are certified for a specific device:

* `Thread CIDs for nRF5340`_
* `Thread CIDs for nRF52840`_
* `Thread CIDs for nRF52833`_
