.. _fprotect_readme:

Hardware flash write protection
###############################

.. contents::
   :local:
   :depth: 2

The hardware flash write protection driver (``fprotect``) can be used to protect flash areas from writing.
The driver uses a hardware peripheral (ACL, BPROT, RRAMC, or SPU, depending on the chip model) to protect the area.
The protection is irreversible until a reset occurs.

Configuration
*************

To use the hardware flash write protection driver, enable the :kconfig:option:`CONFIG_FPROTECT` Kconfig option.

For additional configuration, see the `CONFIG_FPROTECT_*`_ Kconfig options.

Usage example
*************

The following example shows how to protect the ``b0_partition`` flash area:

.. code-block:: c

   #include <zephyr/storage/flash_map.h>

   int err = fprotect_area(PARTITION_ADDRESS(b0_partition),
			   PARTITION_SIZE(b0_partition));

API documentation
*****************

| Header file: :ncs-file:`/include/fprotect.h`
| Source files: :file:`lib/fprotect/`

.. doxygengroup:: fprotect
