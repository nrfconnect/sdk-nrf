.. _ug_nrf54h20_ironside_services:

|ISE| services
##############

.. contents::
   :local:
   :depth: 2

The following are the services provided by |ISE|:

* :ref:`CPUCONF service <ug_nrf54h20_ironside_se_cpuconf_service>`
* :ref:`Update service <ug_nrf54h20_ironside_se_update_service>`
* :ref:`Counter service <ug_nrf54h20_ironside_se_counter_service>`

.. _ug_nrf54h20_ironside_se_cpuconf_service:

|ISE| CPUCONF service
*********************

The |ISE| CPUCONF service enables the application core to trigger the boot of another CPU at a specified address.

Specifically, |ISE| sets INITSVTOR of the CPUCONF instance of the processor being booted with the address provided to the IronSide call, and then writes 0x1 to CPUSTART of the CPUCONF instance of the processor being booted to start the target CPU.
When CPUWAIT is enabled in the IronSide service call, the target CPU is stalled by writing 0x1 to CPUWAIT of the CPUCONF instance of the processor being booted.

This feature is intended for debugging purposes.

.. note::

   * TASKS_ERASECACHE of the CPUCONF instance of the processor being booted is not yet supported.
   * INITNSVTOR of the CPUCONF instance of the processor being booted will not be supported.

For details about the CPUCONF peripheral, refer to the nRF54H20 SoC datasheet.

.. _ug_nrf54h20_ironside_se_update_service:

|ISE| update service
********************

|ISE| provides an update service that allows local domains to trigger the :ref:`update process <ug_nrf54h20_ironside_se_update_architecture>` of |ISE| itself.

The update service requires a release of |ISE| or the |ISE| Recovery image to be programmed within a valid memory range that is accessible by the application core.
See :file:`nrf_ironside/update.h` for more details on the supported memory range.

After the application has invoked the service, |ISE| will update on the next system reset.
The update can be verified by checking the listed versions in the :ref:`boot report <ug_nrf54h20_ironside_se_boot_report>` on startup.

.. note::
   Updating through the service is limited to a single image at a time.
   Updating both |ISE| and |ISE| Recovery images requires performing two rounds of the update service procedure, in which the |ISE| Recovery image is updated first.

See the :zephyr:code-sample:`update application <nrf_ironside_update>` sample for an example on calling the service at runtime.

.. _ug_nrf54h20_ironside_se_counter_service:

|ISE| counter service
*********************

The |ISE| counter service provides secure monotonic counters for rollback protection, version tracking, and other security-critical applications that require strictly increasing values.

The header file for this service is :file:`sdk-zephyr/soc/nordic/ironside/include/nrf_ironside/counter.h`.

It provides four independent 32-bit monotonic counters (``IRONSIDE_COUNTER_0`` through ``IRONSIDE_COUNTER_3``).
Each counter can only be set to a value greater than or equal to its current value, which ensures that counter values never decrease.

The counters have the following properties:

.. list-table::
   :header-rows: 1

   * - Property
     - Description
   * - Monotonic
     - Counter values can only increase or stay the same; they cannot decrease.
   * - Persistent
     - Counter values are stored in secure storage and survive reboots.
   * - Per-boot locking
     - Counters can be locked for the current boot session to prevent further modifications.
   * - Automatic initialization
     - Counters are initialized to 0 during the first boot with an unlocked UICR.

Operations
==========

The Counter service provides three primary operations applicable to the counters: ``set``, ``get``, and ``lock``.

Set
---

The ``ironside_counter_set()`` function sets a counter to a specified value.
The new value must be greater than or equal to the current counter value.

The function takes the following parameters:

* ``counter_id`` - Counter identifier (``IRONSIDE_COUNTER_0`` through ``IRONSIDE_COUNTER_3``).
* ``value`` - New counter value.
  It must be greater than or equal to the current value.

The function returns the following values:

* ``0`` on success.
* ``-IRONSIDE_COUNTER_ERROR_INVALID_ID`` if the counter ID is invalid.
* ``-IRONSIDE_COUNTER_ERROR_TOO_LOW`` if the specified value is lower than the current value.
* ``-IRONSIDE_COUNTER_ERROR_LOCKED`` if the counter is locked for this boot session.
* ``-IRONSIDE_COUNTER_ERROR_STORAGE_FAILURE`` if the storage operation failed.

Get
---

The ``ironside_counter_get()`` function retrieves the current value of a counter.

The function takes the following parameters:

* ``counter_id`` - Counter identifier (``IRONSIDE_COUNTER_0`` through ``IRONSIDE_COUNTER_3``).
* ``value`` - Pointer to store the retrieved counter value.

The function returns the following values:

* ``0`` on success.
* ``-IRONSIDE_COUNTER_ERROR_INVALID_ID`` if the counter ID is invalid.
* ``-IRONSIDE_COUNTER_ERROR_STORAGE_FAILURE`` if the storage operation failed or the counter is not initialized.

Lock
----

The ``ironside_counter_lock()`` function locks a counter for the current boot session, which prevents any further modifications until the next reboot.
Lock states are non-persistent and are cleared on reboot.

The function takes the following parameter:

* ``counter_id`` - Counter identifier (``IRONSIDE_COUNTER_0`` through ``IRONSIDE_COUNTER_3``).

The function returns the following values:

* ``0`` on success.
* ``-IRONSIDE_COUNTER_ERROR_INVALID_ID`` if the counter ID is invalid.

Usage
=====

The Counter service is typically used for the following purposes:

* *Firmware version tracking* - Store the current firmware version and prevent rollback to older versions.
* *Anti-rollback protection* - Ensure that security-critical updates cannot be reverted.
* *Nonce generation* - Generate unique, strictly increasing values for cryptographic operations.

The following example demonstrates how to use the Counter service to track firmware versions and prevent rollback:

.. code-block:: c

   #include <nrf_ironside/counter.h>

   /* Read the current firmware version counter */
   uint32_t current_version;
   int err = ironside_counter_get(IRONSIDE_COUNTER_0, &current_version);
   if (err != 0) {
       /* Handle error */
   }

   /* Update to the new firmware version */
   uint32_t new_version = 42;
   err = ironside_counter_set(IRONSIDE_COUNTER_0, new_version);
   if (err == -IRONSIDE_COUNTER_ERROR_TOO_LOW) {
       /* Firmware rollback detected - reject update */
   }

   /* Lock the counter to prevent tampering during this boot session */
   ironside_counter_lock(IRONSIDE_COUNTER_0);
