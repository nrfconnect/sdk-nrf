.. _lib_secure_services:

Secure Services
###############

Secure Services are functions implemented in the Secure Firmware (:ref:`secure_partition_manager`), but made available to be called from the Non-Secure Firmware.

Calling functions in this API requires that the service is enabled in the :ref:`secure_partition_manager`.
See :option:`CONFIG_SPM_SECURE_SERVICES` in the :ref:`secure_partition_manager`'s menuconfig.
Some services are enabled by default.

API documentation
*****************

| Header file: :file:`include/secure_services.h`
| Source file: :file:`subsys/spm/secure_services.c` (compiled into the :ref:`secure_partition_manager`)

.. doxygengroup:: secure_services
   :project: nrf
   :members: