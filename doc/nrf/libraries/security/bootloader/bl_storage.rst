.. _doc_bl_storage:

Bootloader storage
##################

.. contents::
   :local:
   :depth: 2

The bootloader storage library is used to read and write both one-time programmable (OTP) data and a non-volatile counter.

If the |NSIB| (NSIB) is enabled, it should be the only user of this library.
The application image can only include this library when the NSIB is disabled.

The library has the following functions for either reading or writing, or in some cases both reading and writing:

* The lifecycle state of the device, used to deny certain operations when in wrong lifecycle state.
* A 32-byte user-defined implementation ID, used to identify the immutable bootloader.
* Image slot addresses.
* A monotonic counter, used to enforce anti-rollback protection.
* Hashes of public keys.
* Invalidation tokens, used to revoke public keys.
* Additional public key metadata.

The library uses either the OTP region of the user information configuration registers (UICR), when present on nRF91 Series or nRF5340 devices, or the internal flash memory.
When the library uses the internal flash memory, the bootloader blocks the write access before booting the next image.

See :ref:`bootloader_provisioning` for more information about the provisioned data and how the bootloader uses it.

See :file:`tests/subsys/bootloader/bl_storage/` for tests of the library.

API documentation
*****************

| Header file: :file:`include/bl_storage.h`
| Source files: :file:`subsys/bootloader/bl_storage/`

.. doxygengroup:: bl_storage
   :project: nrf
   :members:
