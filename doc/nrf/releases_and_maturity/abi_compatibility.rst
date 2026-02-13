.. _abi_compatibility:

|ISE| ABI compatibility
#######################

.. contents::
   :local:
   :depth: 2

Application Binary Interface (ABI) compatibility defines how software components, such as libraries and applications, interact at the machine code level through their low-level binary interface.
This includes:

* Function-calling conventions
* Data structure layouts in memory
* Exception handling mechanisms
* Register usage conventions

When ABI compatibility is maintained, binaries of one component can interface correctly with another without requiring recompilation.
For example, adding a new function to a library is typically an ABI-compatible change, as existing binaries remain functional.
However, changes that affect data structure layouts, such as altering field order or size, break ABI compatibility because they change the memory layout expected by existing binaries.

This page describes the ABI compatibility between the |NCS| and the |ISE| binaries.

ABI compatibility for the nRF54H20 IronSide SE binaries
*******************************************************

To use the most recent version of the |NCS|, *always* download and provision your nRF54H20 SoC-based device with the `latest nRF54H20 IronSide SE binaries`_ available.

For information on legacy versions of the nRF54H20 SoC binaries based on SUIT, see :ref:`abi_compatibility_legacy`.

.. caution::
   The nRF54H20 IronSide SE binaries do not support rollbacks to previous versions.

Provisioning the nRF54H20 SoC
*****************************

To provision the nRF54H20 SoC using the nRF54H20 IronSide SE binaries, see :ref:`ug_nrf54h20_gs_bringup`.

Updating the nRF54H20 SoC
*************************

To update the nRF54H20 IronSide SE binaries to the latest version, see :ref:`ug_nrf54h20_ironside_se_update`.

nRF54H20 IronSide SE binaries changelog
***************************************

The following sections provide detailed lists of changes by component.

IronSide Secure Element (IronSide SE) v23.2.1+25
================================================

Added
-----

* Added support for *event reports* and the *event enable* service.
  These features provide mechanisms for forwarding error events from the SPU, MPC, and MRAMC peripherals to local domains. (NCSDK-32170)
* The System Controller now periodically monitors for stalled transactions targeting MRAM, and triggers a global reset to recover from this condition.

Updated
-------

* SysCtrl updated to v6.1.0.
* Enabling radio upscale mode now additionally requests operation with no MRAM latency.
* Disabled data prefetching in the L2CACHE.

Fixed
-----

* An issue with corrupted temperature measurements when sampled too soon after boot.
* PSA Vendor key ID ``0x7fffc001`` now refers to the correct IAK per local domain. (NRFX-8427)

IronSide Secure Element (IronSide SE) v23.1.2+21
================================================

Fixed
-----

* An issue in the temperature service that could delay sending temperature responses. (NCSDK-36336)

Updated
-------

* Reduced MRAM latency when requesting ``no latency`` while MRAM was powered off by powering MRAM on immediately. (NRFX-8740)

IronSide Secure Element (IronSide SE) v23.1.1+20
================================================

Fixed
-----

* L2CACHE now prefetches several cache lines when an L2 cache miss occurs. (HM-26346)
  This improves L2 cache performance in some situations and reduces it in others.
* Fixed an unintentional behavior of ``psa_copy_key()`` when used to create a revocable key. (NCSDK-36369)

IronSide Secure Element (IronSide SE) v23.1.0+19
================================================

Added
-----

* LFXO external square support in SysCtrl.
* Counter service for monotonic counters with PSA ITS storage backend.

  .. note::
     This service can only be used on devices that have booted at least once with an unlocked UICR with this version.

* The IronSide boot reports now contain a 16-byte UUID extracted from the OTP. (NRFX-8171)
* Defined a new category of platform keys, called revocable keys. (NCSDK-35397)

  These are general-purpose, persistent keys which can be provisioned using the PSA Crypto API, but only when the UICR is unlocked.
  When the UICR is locked, destroying a revocable key will prevent it from being created again.
  Using these keys does not depend on the ``UICR.SECURESTORAGE`` configuration.

Updated
-------

* The MRAMC.READY/READYNEXT registers are now readable by local domains. (NCSDK-35534)

  This allows local domains to check if MRAM is ready for a write operation before triggering it.
* The IronSide SE update now fails if it is placed outside the valid memory range (0x0e100000 - 0x0e200000). (NCSDK-35750)
* The IronSide SE PSA crypto service now supports 3 concurrent crypto operations. (NCSDK-35671)

  This enables support for TLS.
* An invalid enumeration for the processor in UICR.SECONDARY.PROCESSOR is now reported with a uicr_regid equal to the offset of that register.
* The NRFS (SysCtrl) IPC buffers for the Application core and Radio core can now only be accessed when the secure attribute is set.
* SysCtrl updated to v6.0.1.
* SysCtrl has updated calibration thresholds for LFRC.

IronSide Secure Element (IronSide SE) v23.0.2+17
================================================

Added
-----

* SHA1 support. (NCSDK-35321)
  This feature corresponds to the ``PSA_WANT_ALG_SHA_1`` CRACEN PSA cryptographic primitive.

  For more information, see :ref:`ug_crypto_supported_features`.


IronSide Secure Element (IronSide SE) v23.0.1+16
================================================

Changed
-------

* The domain-specific built-in keys identified by ``CRACEN_BUILTIN_*_ID``. (NCSDK-35202)
* The way IronSide SE treats the ``UICR.VERSION`` field. (NCSDK-35253)

  The erase value is now interpreted as the highest supported UICR format version.
  Other values must match a supported version or cause a boot error.
  Currently, only version 2.0 (``0x00020000``) is supported.
* IronSide SE no longer disables RETAIN for every GPIO pin at boot. (NCSDK-35080)

  Pins are now retained when the application boots, and the application must disable retention before using them.
* ``UICR.LOCK`` can now be set in ``LCS_EMPTY`` without hindering LFCLK calibration. (NCSDK-34458)

Fixed
-----

* EXMIF XIP region is now accessible. (NCSDK-35256)

IronSide Secure Element (IronSide SE) v23.0.0+15
================================================

Added
-----

* IronSide SE now supports the following CRACEN PSA cryptographic primitives:

  * ``PSA_WANT_GENERATE_RANDOM``
  * ``PSA_WANT_ALG_CTR_DRBG``
  * ``PSA_WANT_ALG_CBC_PKCS7``
  * ``PSA_WANT_ALG_CBC_NO_PADDING``
  * ``PSA_WANT_ALG_CCM``
  * ``PSA_WANT_ALG_CHACHA20_POLY1305``
  * ``PSA_WANT_ALG_CMAC``
  * ``PSA_WANT_ALG_CTR``
  * ``PSA_WANT_ALG_DETERMINISTIC_ECDSA``
  * ``PSA_WANT_ALG_ECB_NO_PADDING``
  * ``PSA_WANT_ALG_ECDH``
  * ``PSA_WANT_ALG_ECDSA``
  * ``PSA_WANT_ALG_ECDSA_ANY``
  * ``PSA_WANT_ALG_GCM``
  * ``PSA_WANT_ALG_HKDF``
  * ``PSA_WANT_ALG_HMAC``
  * ``PSA_WANT_ALG_JPAKE``
  * ``PSA_WANT_ALG_PBKDF2_HMAC``
  * ``PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128``
  * ``PSA_WANT_ALG_PURE_EDDSA``
  * ``PSA_WANT_ALG_SHA_256``
  * ``PSA_WANT_ALG_SHA_384``
  * ``PSA_WANT_ALG_SHA_512``
  * ``PSA_WANT_ALG_SHA3_256``
  * ``PSA_WANT_ALG_SHA3_384``
  * ``PSA_WANT_ALG_SHA3_512``
  * ``PSA_WANT_ALG_SHAKE256_512``
  * ``PSA_WANT_ALG_SP800_108_COUNTER_CMAC``
  * ``PSA_WANT_ALG_SPAKE2P_HMAC``
  * ``PSA_WANT_ALG_SPAKE2P_CMAC``
  * ``PSA_WANT_ALG_SPAKE2P_MATTER``
  * ``PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS``
  * ``PSA_WANT_ALG_TLS12_PRF``
  * ``PSA_WANT_ALG_TLS12_PSK_TO_MS``
  * ``PSA_WANT_ALG_HKDF_EXTRACT``
  * ``PSA_WANT_ALG_HKDF_EXPAND``
  * ``PSA_WANT_ALG_ED25519PH``
  * ``PSA_WANT_ECC_MONTGOMERY_255``
  * ``PSA_WANT_ECC_SECP_R1_256``
  * ``PSA_WANT_ECC_SECP_R1_384``
  * ``PSA_WANT_ECC_SECP_R1_521``
  * ``PSA_WANT_ECC_TWISTED_EDWARDS_255``
  * ``PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE``
  * ``PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT``
  * ``PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT``
  * ``PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE``
  * ``PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY``
  * ``PSA_WANT_KEY_TYPE_AES``
  * ``PSA_WANT_AES_KEY_SIZE_128``
  * ``PSA_WANT_AES_KEY_SIZE_256``
  * ``PSA_WANT_KEY_TYPE_CHACHA20``
  * ``PSA_WANT_KEY_TYPE_PASSWORD``
  * ``PSA_WANT_KEY_TYPE_PASSWORD_HASH``
  * ``PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE``
  * ``PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT``
  * ``PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT``
  * ``PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_GENERATE``
  * ``PSA_WANT_KEY_TYPE_SPAKE2P_PUBLIC_KEY``

  For more information, see :ref:`ug_crypto_supported_features`.

* Support for a secondary boot mode. (NCSDK-32171)

  The secondary mode lets you define a separate application firmware that is started on boot error or when requested over IPC.
  This is configured through the ``UICR.SECONDARY`` registers and can be used for recovery or updates.
* Support for ``UICR.WDTSTART``, which can be used to automatically start a local domain watchdog ahead of the application boot. (NCSDK-35046)
* Support for PSA Internal Trusted Storage (ITS). (NCSDK-18548)

  It is configured through the following ``UICR.SECURESTORAGE`` registers:

  * ``UICR.SECURESTORAGE.CRYPTO`` - Enables persistent key storage for the PSA Crypto API.
  * ``UICR.SECURESTORAGE.ITS`` - Enables the PSA ITS API for managing other sensitive assets.
  * ``UICR.SECURESTORAGE.ENABLE`` and ``UICR.SECURESTORAGE.ADDRESS`` - Required to enable one or both features.

* Support for the IronSide SE DVFS service, replacing the NRFS DVFS service. (NRFX-7321)

Updated
-------

* Renamed the release artifact from :file:`sysctrl.hex` to :file:`ironside_se.hex` to correctly reflect its content.

Removed
-------

* NRFS DVFS service support.

IronSide Secure Element (IronSide SE) v22.2.0+14
================================================

Added
-----

* Support for disabling and enabling USB D+ pull-up control to ensure VBUS-detected IRQs are received.

Changed
-------

* Enabled IPC unbound feature.
* Improved power consumption.
* Improved stability.

Removed
-------

* Temperature subscription

IronSide Secure Element (IronSide SE) v22.1.0+13
================================================

* Added support for configuring TDD (CoreSight++) from local domains. (NCSDK-33486)

IronSide Secure Element (IronSide SE) v22.0.4+12
================================================

Fixed
-----

* An issue where the device became stuck in recovery mode after performing a recovery upgrade. (NCSDK-34258)
* An issue where the eraseall operation, on a device in LCS ROT, was permitted regardless of the contents of UICR. (NCSDK-34232)
* An issue where the recovery firmware would incorrectly determine that UICR was corrupted. (NCSDK-32241)

Updated
-------

* sysctrl to 5.0.1: stability improvements.

IronSide Secure Element (IronSide SE) v22.0.3+11
================================================

Fixed
-----

* psa_crypto:

   * Bytes written are now correctly returned (in place of buffer size). (NCSDK-34172)
   * Added missing ECC_MONTGOMERY_255 configuration. (NCSDK-34200)
   * Passing 0-sized buffers are now allowed for optional arguments. (NCSDK-34171).

* The default owner ID of some peripherals, where previously it was set to SECURE or SYSCTRL instead of APPLICATION. (NCSDK-34187)

IronSide Secure Element (IronSide SE) v22.0.2+10
================================================

* Fixed missing CIPHER support in the PSA crypto service.

IronSide Secure Element (IronSide SE) v22.0.1+9
===============================================

No functional change.
Version bump to verify update with live versions.

IronSide Secure Element (IronSide SE) v22.0.0+8
===============================================

Added
-----

* This release is now signed with Nordic keys.
  The SWD connection is still required to update IronSide using official tools.
  For more information, run ``west ncs-ironside-se-update --help``.
  A backward LCS transition is not required to update IronSide.
* Added support for ``UICR.PROTECTEDMEM``, which enables integrity checking of an immutable bootloader.

Updated
-------

* Increased the size of USLOT (IronSide + sysctrl) to 120 kB.
* Increased the size of RSLOT (IronSide recovery firmware) to 20 kB.
* Enabled downgrade protection for IronSide in debug builds.
* Changed the owner ID used in the default global domain SPU configurations from ``NONE`` to ``APPLICATION``.
  This means that all peripherals and split-ownership registers are accessible by the application core, PPR and FLPR by default.
  Use ``UICR.PERIPHCONF`` to grant the radio core access to global domain peripherals.

IronSide Secure Element (IronSide SE) v21.1.1
=============================================

* Updated to not require CHIDX values to be set when configuring ``PPIB_SUBSCRIBE_SEND`` or ``PPIB_PUBLISH_RECEIVE`` through ``PERIPHCONF``.
* Fixed an issue where the application core was booted despite the presence of boot errors.

IronSide Secure Element (IronSide SE) v21.1.0
=============================================

Added
-----

* MAC in the PSA Crypto service.
* Static memory checks that protect Nordic assets by whitelisting only memory ranges available to the application developers.
* System Controller Firmware (SCFW) releases in the IronSide SE releases.
  See :ref:`scfw_5_0_0` for details.

Updated
-------

* The CPUCONF service request definition.
* The CPU and WAIT parameters are now both packed into the first 4-byte value, and the message data is sent inline in the request.
* Support for initializing a subset of global domain peripherals by configuring ``UICR.PERIPHCONF``.
  This enables the initial configuration of the CTRLSEL GPIO pin, global IRQ mapping, IPC mapping, global PPIBs, and more.
* ``UICR.PERIPHCONF`` reintroduces functionality that was previously available with specialized ``UICR.*`` registers, but with a lower-level interface that is more powerful, flexible, and future proof.

.. _scfw_5_0_0:

System Controller Firmware (SCFW) v5.0.0
----------------------------------------

Added
~~~~~

* SWEXT service.

Updated
~~~~~~~

* Reduced MRAM auto power down timeout (helps with lower power usage).
* GRCCONF module code optimization.
* IPC communication code optimization (Zephyr IPC service used directly without additional queue).
* Fixed higher power consumption when clock switcher changed to LFRC.

IronSide Secure Element (IronSide SE) v21.0.1
=============================================

Added
-----

* Boot report to be written to radio core. (NCSDK-33583)

Updated
-------

* Enable link-time optimization.
* Disable CRACEN microcode loading. (NCSDK-32173)

Fixed
-----

* The application core is now started in halted mode when IronSide SE detects errors in the UICR or BICR. (NCSDK-33306)
  This allows recovery from such errors by writing correct values using a debugger.

IronSide Secure Element (IronSide SE) v21.0.0
=============================================

Added
-----

* Support for the IronSide SE update service. (NCSDK-32173)
  This service allows updating IronSide SE firmware using the ``west ncs-ironside-se-update`` command.
  The update is performed over SWD, and the device must be in a debug mode.
* Experimental support for a new UICR format. (NCSDK-32444)
  At this stage, the functionality is mainly for internal testing and development, and user tools for interacting with UICR will be added at a later stage.
* Boot report support. (NCSDK-32393)
* CPUCONF service for booting the radio core. (NCSDK-32925)
  Currently, only ``hello world`` is supported.
* IronSide calls, the successor to SSF. (NCSDK-32441).

Updated
-------

* The limited PSA Crypto API is now implemented as an IronSide call. (NCSDK-32912)
  This replaces the temporary IPC mechanism from the last release.

Fixed
-----

* An issue that set the CTRLAP.BOOTSTATUS firmware sequence number always to zero. (NCSDK-33265)

IronSide Secure Element (IronSide SE) v20.1.0
=============================================

Added
-----

* Added experimental support for a limited :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` service.
  This is built on top of a temporary IPC mechanism which will soon be replaced.
  The top-level interface will remain the same. (NCSDK-32163)

  The PSA Crypto API support through the :ref:`ug_crypto_architecture_implementation_standards_ironside` is currently limited to the following ``PSA_WANT`` symbols for :ref:`cryptographic feature selection <crypto_drivers_feature_selection>`:

  * ``PSA_WANT_GENERATE_RANDOM``
  * ``PSA_WANT_ALG_SHA_256``
  * ``PSA_WANT_ALG_SHA_512``
  * ``PSA_WANT_ALG_GCM``
  * ``PSA_WANT_ALG_ECDSA``
  * ``PSA_WANT_ALG_ECDH``
  * ``PSA_WANT_ALG_ED25519PH``
  * ``PSA_WANT_ECC_SECP_R1_256``
  * ``PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY``
  * ``PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT``
  * ``PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT``
  * ``PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE``
  * ``PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE``

  To use the service, set the :kconfig:option:`CONFIG_NRF_SECURITY` to ``y``.
  For more information on the implementations available in the cryptographic drivers, see the :ref:`crypto_drivers`.

  .. note::
     The v20.1.0 support for this PSA Certified Crypto API is not compatible with |NCS| v3.0.0 or v3.1.0.
     It is only compatible with the ``sdk-nrf`` revision ``1b2abb07b8b2528ecaf86f54e0c6cf48c425055a``.

Updated
-------

* ``EXTRAVERSION`` is now included in ``SICR.TBS.x.VERSION``.

IronSide Secure Element (IronSide SE) v20.0.1
=============================================

Updated
-------

* AUX-AP to be always disabled.
* Internal optimization of MRAM and RAM usage.

Removed
-------

* Initializing TDD on system boot as it is not retained.
* Initializing P6 and P7 pins to be EXMIF/TPIU to prevent unacceptable output states.

IronSide Secure Element (IronSide SE) v20.0.0
=============================================

This is the first release that is based on the new Secure Domain firmware architecture.
Most of the functionality from the preceding version of SDFW has been disabled or removed and will be gradually reintroduced in upcoming versions.

Added
-----

* Support for the ``ERASEALL`` command through the boot command interface. (NCSDK-31997)
* Support for the ``DEBUGWAIT`` command through the boot command interface.
* A new scheme for status reporting through the BOOTSTATUS register in CTRL-AP. (NCSDK-32355)

Updated
-------

* SCFW to be included in the URoT firmware partition.
  Additionally, the SysCtrl CPU is always started. (NCSDK-31993)
* SDFW to not start the radio core.
  The application core is now always started with the secure VTOR set to the first address following the IronSide SE partitions. (NCSDK-31995)
* SDFW to statically configure the device at boot so that most resources are accessible by the application core without needing to modify the UICR. (NCSDK-31999)

Removed (from legacy SUIT-based SDFW)
-------------------------------------

* SSF and all SSF services have been disabled. (NCSDK-32000)
* Resource configuration based on UICR has been disabled. (NCSDK-31999)
* The SDFW ADAC interface has been disabled. (NCSDK-31994)
* SUIT is no longer supported. (NCSDK-31996)

nRF54H20 SoC binaries v0.9.6 changelog
**************************************

The following sections provide detailed lists of changes by component.

Secure Domain Firmware (SDFW) v10.3.3
=====================================

* Updated BINDESC to a new version.

Secure Domain Firmware (SDFW) v10.3.1
=====================================

Added
-----

* Enabled pulling of Secure Domain images during SUIT manifest processing.

Fixed
-----

* Adjusted file URIs to prevent SUIT envelope size overflow.
* Resolved an issue where the IPUC write setup was being erased, ensuring proper SUIT AB operation.

System Controller Firmware (SCFW) v4.2.3
=========================================

* Removed changing ``VREG1V0 VOUT`` for the high-power radio in power management temperature monitoring.
  The actual value is now set by the SysCtrl ROM from FICR.

System Controller Firmware (SCFW) v4.2.1
=========================================

* Updated PCRM configuration to set the BLE active parameter to ``0x0E``.

System Controller Firmware (SCFW) v4.2.0
=========================================

* Updated the ``PCRM.LOAD`` value for radio on-demand operations using dedicated VEVIF channels.
* Implemented a workaround for ICPS-1304.

System Controller Firmware (SCFW) v4.1.0
=========================================

Added
-----

* Audio PLL service for local domains.
* LFRC support.

Updated
-------

* Clock initialization tree to support a new 32k clock source - LFRC.

Removed
-------

* Split image partition.

nRF54H20 SoC binaries v0.9.2 changelog
**************************************

The following sections provide detailed lists of changes by component.

Secure Domain Firmware (SDFW) v10.2.0
=====================================

* Updated SUIT to support defining the SUIT cache in Nordic manifests.

Secure Domain Firmware (SDFW) v10.1.0
=====================================

Added
-----

* GPIO DRIVECTRL for P6 and P7 on nRF54H20 is now corrected by SDFW on boot.
  This addresses an issue where some devices has this incorrectly configured.
* Added support for TLS-1.3 in the PSA crypto service.
* Added support for ED25519 pre-hashed in the PSA crypto service.
* The SDFW now uses a watchdog timer with a timeout of 4 seconds.
* Purge protection can be enabled over ADAC.
* Clock control is enabled in SDFW.
* Global domain power request service is integrated in SDFW.
* PUF values from SDROM are cleared on boot.

Updated
-------

* A local domain reset now triggers a global reset.
  ``RESETINFO`` contains both the global and local reset reasons.
* All processors now boot regardless of whether they have firmware.
  If no firmware is present, they boot in halted mode.
* Reduced power consumption from the Secure Domain when tracing is enabled.
* Increased the number of possible concurrent PSA operations to 8.
* The ETR buffer location is now read from the UICR.
  Enabling ETR tracing now requires configuring the location.
* The SDFW no longer immediately resets on a fatal error.

Removed
-------

* Several services from SSF over ADAC.
* Reset event service.

Fixed
-----

* An issue where SDFW exited the sleep state for a short duration after boot completion.
* An issue where replies to ADAC SSF commands contained a large amount of additional zero values at the end of the message.
* An issue where permission checks for pointer members in the SSF PSA crypto service requests were incorrect.
* An issue with invoking crypto service from multiple threads or clients.

System Controller Firmware (SCFW) v4.0.3
=========================================

* Updated LRC to now use a direct GDPWR request.
* Fixed an issue with USB D+ pull-up.

System Controller Firmware (SCFW) v4.0.2
=========================================

Added
-----

* GDFS service: New service implementation to handle change of global domain frequency on demand (HSFLL120).
* GDPWR service: New power domains.

Updated
-------

* Improved stability.
* GDPWR service: Renamed power domains.
* GPIO power configuration:

  * When ``POWER.CONFIG.VDDAO1V8 == External``, the function ``power_bicr_is_any_gpio_powered_from_internal_1v8_reg`` now returns ``false``.
    This allows proper selection of low power modes when supplying nRF54H20 with an external 1.8V, even if the ``VDDIO_x`` are configured as SHORTED.

* Temperature sensor coefficients.

.. _abi_compatibility_legacy:

Legacy ABI compatibility matrix for the nRF54H20 SoC binaries
=============================================================

The following table illustrates the legacy ABI compatibility between SUIT-based (pre-IronSide SE) nRF54H20 SoC binaries and older versions of the |NCS|:

.. caution::
   * Devices already provisioned using SUIT-based SoC binaries and in LCS ``RoT`` cannot be upgraded to IronSide SE.

.. list-table::
   :header-rows: 1

   * - |NCS| versions
     - Compatible nRF54H20 SoC binaries version based on SUIT
       (no longer usable with the newest |NCS| versions)
   * - |NCS| v3.0.0
     - nRF54H20 SoC binaries v0.9.6, compatible with the nRF54H20 DK v0.9.0 and later DK revisions.
   * - |NCS| v2.9.0-nRF54H20-1
     - nRF54H20 SoC binaries v0.9.2, compatible with the nRF54H20 DK v0.9.0 and later DK revisions.
   * - |NCS| v2.9.0
     - nRF54H20 SoC binaries v0.7.0 for EngC DKs, compatible with the nRF54H20 DK v0.8.3 and later DK revisions.
   * - |NCS| v2.8.0
     - nRF54H20 SoC binaries v0.7.0 for EngC DKs, compatible with the nRF54H20 DK v0.8.3 and later DK revisions.
       nRF54H20 SoC binaries v0.7.0 for EngB DKs, compatible with the nRF54H20 DKs ranging from v0.8.0 to v0.8.2.
   * - |NCS| v2.7.99-cs2
     - nRF54H20 SoC binaries v0.6.5
   * - |NCS| v2.7.99-cs1
     - nRF54H20 SoC binaries v0.6.2
   * - |NCS| v2.7.0
     - nRF54H20 SoC binaries v0.5.0
   * - |NCS| v2.6.99-cs2
     - nRF54H20 SoC binaries v0.3.3
