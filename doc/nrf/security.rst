.. _security:
.. _security_index:

Security
########

This section provides an overview of core security features available in Nordic Semiconductor products.
The features are made available either as built-ins in modules, drivers, and subsystems, or are shown in samples or applications in |NCS|.

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
    - The |NCS| supports :ref:`MCUboot and nRF Secure Immutable Bootloader (NSIB) <ug_bootloader_mcuboot_nsib>` for secure boot, and DFU procedures using MCUboot and :ref:`Software Updates for Internet of Things (SUIT) <ug_nrf54h20_suit_dfu>`.
    - See :ref:`app_bootloaders`.
    - | - :ref:`nRF Secure Immutable Bootloader (NSIB) <bootloader>`
      | - :ref:`DFU libraries <lib_dfu>`
      | - :ref:`Software Updates for Internet of Things (SUIT) <ug_nrf54h20_suit_dfu>`
      | - :doc:`MCUboot <mcuboot:design>`
  * - Processing environments (CMSE)
    - The :ref:`boards supported by the SDK <app_boards_names>` distinguish entries according to which CPU is to be targeted (for multi-core SoCs) and whether Cortex-M Security Extensions (CMSE) are used or not.
      When CMSE is used, the firmware is split in accordance with the security by separation architecture principle to better protect sensitive assets and code.
      In the |NCS|, the CMSE support is implemented using Trusted Firmware-M (TF-M).
    - See :ref:`app_boards_spe_nspe`.
    - All samples and applications that support the ``*/ns`` :ref:`variant <app_boards_names>` of the boards.
  * - Trusted Firmware-M (TF-M)
    - TF-M is the reference implementation of `Platform Security Architecture (PSA)`_.
      On nRF5340, nRF54L and nRF91 Series devices, TF-M is used to configure and boot an application with :ref:`CMSE enabled <app_boards_spe_nspe_cpuapp_ns>`.
    - See :ref:`ug_tfm`.
    - | - :ref:`tfm_samples`
      | - :ref:`crypto_samples`
      | - :ref:`TF-M integration samples <zephyr:tfm_integration-samples>` in Zephyr
  * - Cryptographic operations (:ref:`nrf_security`)
    - The :ref:`nrf_security` library acts as an orchestrator for the different cryptographic libraries available in the system.
      HW accelerated libraries are prioritized over SW libraries when both are enabled.
      | Find more information on nRF54L Series-specific cryptography operations and the related configuration in :ref:`ug_nrf54l_cryptography`.
    - :kconfig:option:`CONFIG_NRF_SECURITY` (:ref:`more info<nrf_security_config>`)
    - | - :ref:`nrf_security` library with :ref:`nrf_security_drivers`
      | - :ref:`nrfxlib:crypto`
      | - :ref:`crypto_samples`
  * - Trusted storage
    - The trusted storage library enables you to provide features like integrity, confidentiality and authenticity of the stored data, without using the TF-M Platform Root of Trust (PRoT).
    - See :ref:`trusted_storage_in_ncs` and :ref:`trusted storage library configuration <trusted_storage_configuration>`.
    - :ref:`trusted_storage_readme` library
  * - Hardware unique key (HUK)
    - Nordic Semiconductor devices featuring the CryptoCell cryptographic accelerator allow the usage of a hardware unique key (HUK) for key derivation.
      A HUK is a unique symmetric cryptographic key which is loaded in special hardware registers allowing the application to use the key by reference, without any access to the key material.
    - :kconfig:option:`CONFIG_HW_UNIQUE_KEY`
    - | - :ref:`HUK library <lib_hw_unique_key>`
      | - :ref:`HUK sample <hw_unique_key_usage>`

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   security/ap_protect
   security/tfm
   security/trusted_storage
