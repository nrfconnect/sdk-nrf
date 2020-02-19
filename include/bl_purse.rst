.. _doc_bl_purse:

Bootloader purse
################

The bootloader purse library is used by the :ref:`bootloader` to read from and write to a protected data area containing its provisioned data and monotonic version counter.

The library can also be included into the app if it needs to read the bootloader's purse.
The purse is placed in the 'OTP' region of UICR if available.
Otherwise, the purse is placed in a regular flash page, in which case the bootloader will block write access to it before booting the next image.
When the purse is write-protected, some functions do not work when they are called from the app.
The purse cannot be read from non-secure apps.

The purse contains:
 - Image slot sizes
 - Public keys
 - Invalidation tokens used to revoke public keys
 - Monotonic version counter

See :ref:`bootloader_provisioning` for more information about the provisioned data and how the bootloader uses it.

There are tests for the library at :file:`tests/subsys/bootloader/bl_purse/`.

Monotonic version counter
*************************

The purse can contain a monotonic version counter for the app.
This is implemented as a series of 16 bit values, where each update to the counter is written to the next available slot.
When reading the counter, the bootloader purse library iterates over each slot checking that each is larger than the previous, to guarantee that it is monotonic (always increases).

The number of available slots is configurable via :option:`CONFIG_SB_NUM_MONOTONIC_COUNTERS`.
The monotonic counter can be disabled via :option:`CONFIG_SB_MONOTONIC_COUNTER`.

If the counter is enabled, :ref:`doc_bl_validation` checks it against an image's version during :cpp:func:`bl_validate_firmware`.

API documentation
*****************

| Header file: :file:`include/bl_purse.h`
| Source files: :file:`subsys/bootloader/bl_purse/`

.. doxygengroup:: bl_purse
   :project: nrf
   :members:
