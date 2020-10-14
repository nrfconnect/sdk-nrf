.. _doc_bl_storage:

Bootloader storage
##################

.. contents::
   :local:
   :depth: 2

The bootloader storage library is used by the :ref:`bootloader` to read from and write to a protected data area that contains the provisioned bootloader data.
The library can also be included into the application to allow reading the provisioned data.

The provisioned data is stored in the one-time programmable (OTP) region of UICR, if available (for example, on nRF9160 and the nRF5340 application core).
In this case, it can only be read from applications that are in the secure domain.
If OTP is not available, the data is stored in a regular flash page, and the bootloader blocks write access to it before booting the next image.
In this case, some functions do not work when they are called from the application.

The library stores the following data:

* Image slot sizes
* Hashes of public keys
* Invalidation tokens used to revoke public keys
* :ref:`Application versions <store_app_version>`


See :ref:`bootloader_provisioning` for more information about the provisioned data and how the bootloader uses it.

You can find tests for the library at :file:`tests/subsys/bootloader/bl_storage/`.

.. _store_app_version:

Storing version information
***************************

The bootloader storage can be used to keep track of the version of the application.
When updating the application, the :ref:`doc_bl_validation` library checks if the new version is newer than the current version and only accepts the new image if it is.
After updating, the application must then store the new version information in the bootloader storage.

This functionality is implemented as a monotonic version counter that contains a series of 16-bit integer values.
Each update to the counter is written to the next available slot.
When reading the counter, the bootloader storage library iterates over each slot and returns the largest value.

The number of available slots, thus the number of different version numbers that can be stored, is configurable through :option:`CONFIG_SB_NUM_VER_COUNTER_SLOTS`.

The monotonic counter is enabled by default.
You can disable it through :option:`CONFIG_SB_MONOTONIC_COUNTER`.
If the counter is enabled, the :ref:`doc_bl_validation` library checks it against an image's version during :c:func:`bl_validate_firmware`.


API documentation
*****************

| Header file: :file:`include/bl_storage.h`
| Source files: :file:`subsys/bootloader/bl_storage/`

.. doxygengroup:: bl_storage
   :project: nrf
   :members:
