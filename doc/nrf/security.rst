.. _security:
.. _security_index:

Security
########

This section provides an overview of core security features available in Nordic Semiconductor products.
The features are made available either as built-ins in modules, drivers, and subsystems, or are shown in samples or applications in |NCS|.

.. security_components_ver_table_start

The |NCS| |release| allows you to develop applications with the following versions of security components:

.. list-table:: |NCS|, TF-M, IronSide Secure Element, and Mbed TLS versions
     :header-rows: 1
     :widths: auto

     * - |NCS| release
       - TF-M version
       - IronSide Secure Element version
       - Mbed TLS version
     * - |release|
       - v2.2.0
       - v23.1.0+19
       - 3.6.5

.. security_components_ver_table_end

Expand the following section to see the table listing versions of different security components implemented since the |NCS| v2.1.0.

.. toggle::

   .. note::

      Not all `official TF-M releases`_ are implemented by the |NCS|.
      This is because the |NCS| implements TF-M through Zephyr.
      Zephyr adds specific patches to the TF-M version, which are then upmerged into the |NCS| with changes specific to the |NCS|.

      Similarly, not all `official Mbed TLS releases`_ are implemented by the |NCS| through the `sdk-mbedtls`_ repository.

   .. list-table:: |NCS|, TF-M, and Mbed TLS versions
     :header-rows: 1
     :widths: auto

     * - |NCS| release
       - TF-M version
       - IronSide Secure Element version
       - Mbed TLS version
     * - Upcoming release (currently on the ``main`` branch of `sdk-nrf`_)
       - v2.2.0
       - v23.0.2+17
       - 3.6.5
     * - v3.2.0
       - v2.2.0
       - v23.1.0+19
       - 3.6.5
     * - v3.1.0, v3.1.1
       - v2.1.2
       - n/a
       - 3.6.4
     * - v3.0.0 (up to v3.0.2)
       - v2.1.1
       - n/a
       - 3.6.3
     * - v2.9.0 (up to v2.9.2)
       - v2.1.1
       - n/a
       - 3.6.2
     * - v2.8.0
       - v2.1.1
       - n/a
       - 3.6.2
     * - v2.7.0
       - v2.0.0
       - n/a
       - 3.5.2
     * - v2.6.0 (up to v2.6.4)
       - v2.0.0
       - n/a
       - 3.5.2
     * - v2.5.0 (up to v2.5.3)
       - v1.8.0
       - n/a
       - 3.3.0
     * - v2.4.0 (up to v2.4.4)
       - v1.7.0
       - n/a
       - 3.3.0
     * - v2.3.0
       - v1.6.0
       - n/a
       - 3.1.0
     * - v2.2.0
       - v1.6.0
       - n/a
       - 3.1.0
     * - v2.1.0 (up to v2.1.4)
       - v1.6.0
       - n/a
       - 3.1.0

The following table lists the available general security features.
Some of them are documented in detail in other parts of this documentation, while others are documented in the subpages in this section.

 .. list-table::
  :header-rows: 1

  * - Security feature
    - Description
    - Configuration
    - Related components
  * - Access port protection (AP-Protect)
    - When enabled, this mechanism blocks the debugger from read and write access to all CPU registers and memory-mapped addresses.
    - See :ref:`app_approtect`.
    - ---
  * - Bootloader and Device Firmware Upgrade (DFU)
    - The |NCS| supports :ref:`MCUboot and nRF Secure Immutable Bootloader (NSIB) <ug_bootloader_mcuboot_nsib>` for secure boot, and DFU procedures using MCUboot.
    - See :ref:`app_bootloaders`.
    - | - :ref:`nRF Secure Immutable Bootloader (NSIB) <bootloader>`
      | - :ref:`DFU libraries <lib_dfu>`
      | - :doc:`MCUboot <mcuboot:design>`
  * - Cryptographic operations
    - The |NCS| follows the :ref:`PSA Crypto standard <ug_psa_certified_api_overview_crypto>` and provides :ref:`two different implementations <ug_crypto_architecture_implementation_standards>`, Oberon PSA Crypto and TF-M Crypto Service.
      The :ref:`nrf_security` library acts as an orchestrator for the different cryptographic libraries available in the system.
      HW accelerated libraries are prioritized over SW libraries when both are enabled.
    - :kconfig:option:`CONFIG_NRF_SECURITY` (:ref:`more info<psa_crypto_support>`)
    - | - :ref:`nrf_security` library
      | - :ref:`nrfxlib:crypto`
      | - :ref:`crypto_samples`
      | - :ref:`ug_nrf54l_cryptography`
  * - Trusted Firmware-M (TF-M)
    - TF-M is the reference implementation of `Platform Security Architecture (PSA)`_.
      On :ref:`boards with the /ns variant <app_boards_names>`, TF-M is used to configure and boot an application with :ref:`security by separation <app_boards_spe_nspe_cpuapp_ns>`.
    - See :ref:`ug_tfm`.
    - | - :ref:`tfm_samples`
      | - :ref:`crypto_samples`
      | - :zephyr:code-sample-category:`tfm_integration` in Zephyr
  * - Processing environments (CMSE)
    - The :ref:`boards supported by the SDK <app_boards_names>` distinguish entries according to which CPU is to be targeted (for multi-core SoCs) and whether Cortex-M Security Extensions (CMSE) are used or not.
      When CMSE is used, the firmware is split in accordance with the security by separation architecture principle to better protect sensitive assets and code.
      In the |NCS|, the CMSE support is implemented using Trusted Firmware-M (TF-M).
    - See :ref:`app_boards_spe_nspe`.
    - All samples and applications that support the ``*/ns`` :ref:`variant <app_boards_names>` of the boards.
  * - Secure storage
    - Secure storage enables you to provide features like integrity, confidentiality and authenticity of the stored data, with or without TF-M.
    - See :ref:`secure_storage_in_ncs`.
    - | - :ref:`trusted_storage_readme` library
      | - TF-M's :ref:`ug_tfm_services_its`
      | - TF-M's :ref:`tfm_partition_ps`
  * - Hardware unique key (HUK)
    - Nordic Semiconductor devices featuring the CryptoCell cryptographic accelerator allow the usage of a hardware unique key (HUK) for key derivation.
      A HUK is a unique symmetric cryptographic key which is loaded in special hardware registers allowing the application to use the key by reference, without any access to the key material.
    - :kconfig:option:`CONFIG_HW_UNIQUE_KEY`
    - | - :ref:`HUK library <lib_hw_unique_key>`
      | - :ref:`HUK sample <hw_unique_key_usage>`

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   security/psa_certified_api_overview
   security/crypto/index
   security/tfm/index
   security/ap_protect
   security/secure_storage
   security/key_storage
