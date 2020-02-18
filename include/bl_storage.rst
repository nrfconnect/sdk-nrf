.. _doc_bl_storage:

Bootloader storage
##################

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

See :ref:`bootloader_provisioning` for more information about the provisioned data and how the bootloader uses it.

You can find tests for the library at :file:`tests/subsys/bootloader/bl_storage/`.

API documentation
*****************

| Header file: :file:`include/bl_storage.h`
| Source files: :file:`subsys/bootloader/bl_storage/`

.. doxygengroup:: bl_storage
   :project: nrf
   :members:
