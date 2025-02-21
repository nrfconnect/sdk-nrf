:orphan:

.. _spm_to_tfm_migration:

Migrating from Secure Partition Manager to Trusted Firmware-M
#############################################################

The Nordic Secure Partition Manager (SPM) was replaced with Trusted Firmware-M (TF-M) as the default trusted execution solution in the |NCS| v2.1.0.
This change enhances the security features of the SDK by integrating the more widely adopted TF-M that aligns with the Arm Platform Security Architecture (PSA).

Porting SPM Secure Services
***************************

Migration from SPM to TF-M requires changes in the application code and the partition configuration.
The interface to TF-M is different from the interface to SPM.
Due to that, the application code that uses the SPM Secure Services needs to be ported to use TF-M instead.

TF-M can replace the following SPM services:

.. list-table:: Migrating SPM Secure Services to TF-M
   :widths: 40 60
   :header-rows: 1

   * - SPM Function
     - TF-M Replacement
   * - ``spm_request_system_reboot``
     - ``tfm_platform_system_reset``
   * - ``spm_request_random_number``
     - ``psa_generate_random`` or ``entropy_get_entropy``
   * - ``spm_request_read``
     - ``tfm_platform_mem_read`` or ``soc_secure_mem_read``
   * - ``spm_s0_active``
     - ``tfm_platform_s0_active``
   * - ``spm_firmware_info``
     - ``tfm_firmware_info``


SPM Secure Services with no replacement
***************************************

The following SPM services have no replacement in TF-M:

* ``spm_prevalidate_b1_upgrade``
* ``spm_busy_wait``
* ``spm_set_ns_fatal_error_handler``

Remove these SPM services from your project.

Static partition file edits
***************************

By default, TF-M configures memory regions as secure memory, while SPM configures memory regions as non-secure.
The partitions ``tfm_nonsecure``, ``mcuboot_secondary``, and ``nonsecure_storage`` are configured as non-secure flash memory regions.
The partition ``sram_nonsecure`` is configured as a non-secure RAM region.

If a static partition file is used for the application, make the following changes:

* Rename the ``spm`` partition to ``tfm``.
* Add a partition called ``tfm_secure`` that spans ``mcuboot_pad`` (if MCUboot is enabled) and ``tfm`` partitions.
* Add a partition called ``tfm_nonsecure`` that spans the application, and other possible application partitions that must be non-secure.
* For non-secure storage partitions, place the partitions inside the ``nonsecure_storage`` partition.
