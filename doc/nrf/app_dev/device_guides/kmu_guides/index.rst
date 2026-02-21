.. _ug_kmu_guides:

Working with the KMU and CRACEN
###############################

.. include:: kmu_cracen_overview.rst
   :start-after: .. kmu_cracen_intro_start
   :end-before: .. kmu_cracen_intro_end

The KMU and CRACEN hardware peripherals provide several important benefits for applications developed with the |NCS|:

Secure key storage and provisioning
   The KMU provides hardware-level protection for cryptographic keys, storing them in a dedicated secure region that is isolated from the CPU.
   This is crucial not only for private keys but also for public keys, as the KMU can directly transfer keys to CRACEN's protected RAM.
   Even when keys must pass through addressable RAM, the KMU significantly reduces the risk of key exposure.
   Use KMU for managing secrets whenever possible.

Key provisioning for cryptographic operations
   Applications can provision keys to the KMU for use in cryptographic operations such as encryption, decryption, signing, and verification.
   You can provision keys using external tools or using the PSA Crypto API.
   Once provisioned, using these keys does not expose the key material to the CPU.

Key provisioning for bootloaders
   Bootloaders require cryptographic keys to verify firmware images before booting.
   The KMU can store bootloader verification keys (such as the UROT_PUBKEY for MCUboot or BL_PUBKEY for NSIB) securely in hardware.
   Bootloaders can use multiple key generations (up to three for most nRF54L SoCs) for image verification, allowing for key rotation and revocation.
   It is essential to provision bootloader keys before the first boot, as bootloaders might fail to boot or might take unwanted actions if appropriate keys are not available.
   See :ref:`ug_kmu_provisioning_bootloader_keys` for more information.

Integration with Trusted Firmware-M
   When using an nRF54L device with Trusted Firmware-M, you can use the KMU to store keys instead of using the :ref:`ug_tfm_services_its`.
   This provides hardware-level key protection while maintaining compatibility with TF-M security services.

   Trusted Firmware-M lets you filter some of the KMU keys and keys derived from the :ref:`CRACEN Isolated Key Generator (IKG) <ug_kmu_guides_cracen_ikg>` that are stored in the non-secure environment.
   This way, you can make those filtered-out keys accessible only to code running in the secure processing environment (secure partitions and secure services).

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   kmu_cracen_overview
   kmu_psa_crypto_api_prog_model
   kmu_provisioning_overview
