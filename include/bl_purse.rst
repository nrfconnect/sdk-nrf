.. _doc_bl_purse:

Bootloader purse
################

The bootloader purse library is used by the :ref:`bootloader` to read from and write to a protected data area containing its provisioned data.

The library can also be included into the app if it needs to read the bootloader's purse.
The purse is placed in the 'OTP' region of UICR if available.
Otherwise, the purse is placed in a regular flash page, in which case the bootloader will block write access to it before booting the next image.
When the purse is write-protected, some functions do not work when they are called from the app.
The purse cannot be read from non-secure apps.

The purse contains:
 - Image slot sizes
 - Public keys
 - Invalidation tokens used to revoke public keys

See :ref:`bootloader_provisioning` for more information about the provisioned data and how the bootloader uses it.

There are tests for the library at :file:`tests/subsys/bootloader/bl_purse/`.

API documentation
*****************

| Header file: :file:`include/bl_purse.h`
| Source files: :file:`subsys/bootloader/bl_purse/`

.. doxygengroup:: bl_purse
   :project: nrf
   :members:
