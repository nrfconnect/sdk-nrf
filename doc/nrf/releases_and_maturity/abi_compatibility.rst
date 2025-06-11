.. _abi_compatibility:

ABI compatibility
#################

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

ABI compatibility matrix for the nRF54H20 SoC binaries
******************************************************

The following table illustrates ABI compatibility between different versions of the nRF54H20 SoC binaries and the |NCS|:

.. list-table::
   :header-rows: 1

   * - |NCS| versions
     - Compatible nRF54H20 SoC binaries version
   * - |NCS| v3.1.0
     - `nRF54H20 SoC binaries v22.2.0+14`_, compatible with the nRF54H20 DK v0.9.0 and later revisions.
       It includes IronSide Secure Element (IronSide SE).
   * - |NCS| v3.0.0
     - `nRF54H20 SoC binaries v0.9.6`_, compatible with the nRF54H20 DK v0.9.0 and later revisions.
   * - |NCS| v2.9.0-nRF54H20-1
     - `nRF54H20 SoC binaries v0.9.2`_, compatible with the nRF54H20 DK v0.9.0 and later revisions.
   * - |NCS| v2.9.0
     - `nRF54H20 SoC binaries v0.7.0 for EngC DKs`_, compatible with the nRF54H20 DK v0.8.3 and later revisions.
   * - |NCS| v2.8.0
     - `nRF54H20 SoC binaries v0.7.0 for EngC DKs`_, compatible with the nRF54H20 DK v0.8.3 and later revisions.
       `nRF54H20 SoC binaries v0.7.0 for EngB DKs`_, compatible with the nRF54H20 DKs ranging from v0.8.0 to v0.8.2.
   * - |NCS| v2.7.99-cs2
     - `nRF54H20 SoC binaries v0.6.5`_
   * - |NCS| v2.7.99-cs1
     - `nRF54H20 SoC binaries v0.6.2`_
   * - |NCS| v2.7.0
     - `nRF54H20 SoC binaries v0.5.0`_
   * - |NCS| v2.6.99-cs2
     - `nRF54H20 SoC binaries v0.3.3`_

Maintaining ABI compatibility ensures that the Secure Domain and System Controller firmware binaries do not need to be recompiled each time the application, radio binaries, or both are recompiled, as long as they are based on a compatible |NCS| version.
Additionally, maintaining ABI compatibility allows the nRF54H20 SoC binary components to work together without recompilation when updating to newer |NCS| versions.

.. note::
    The nRF54H20 SoC binaries only support specific versions of the |NCS| and do not support rollbacks to previous versions.
    Upgrading the nRF54H20 SoC binaries on your development kit might break the DK's compatibility with applications developed for earlier versions of the |NCS|.

nRF54H20 SoC binaries v22.2.0+14 changelog
******************************************

The following sections provide detailed lists of changes by component.

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

* Added support for configuring TDD (CoreSight++) from local domains (NCSDK-33486).

IronSide Secure Element (IronSide SE) v22.0.4+12
================================================

Fixed
-----

* An issue where the device became stuck in recovery mode after performing a recovery upgrade (NCSDK-34258).
* An issue where the eraseall operation, on a device in LCS ROT, was permitted regardless of the contents of UICR (NCSDK-34232).
* An issue where the recovery firmware would incorrectly determine that UICR was corrupted (NCSDK-32241).

Updated
-------

* sysctrl to 5.0.1: stability improvements.

IronSide Secure Element (IronSide SE) v22.0.3+11
================================================

Fixed
-----

* psa_crypto:

   * Bytes written are now correctly returned (in place of buffer size) (NCSDK-34172).
   * Added missing ECC_MONTGOMERY_255 configuration (NCSDK-34200).
   * Passing 0-sized buffers are now allowed for optional arguments (NCSDK-34171).

* The default owner ID of some peripherals, where previously it was set to SECURE or SYSCTRL instead of APPLICATION (NCSDK-34187).

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

* Boot report to be written to radio core (NCSDK-33583).

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
* Experimental support for a new UICR format (NCSDK-32444).
  At this stage, the functionality is mainly for internal testing and development, and user tools for interacting with UICR will be added at a later stage.
* Boot report support (NCSDK-32393).
* CPUCONF service for booting the radio core (NCSDK-32925).
  Currently, only ``hello world`` is supported.
* IronSide calls, the successor to SSF (NCSDK-32441).

Updated
-------

* The limited PSA Crypto API is now implemented as an IronSide call (NCSDK-32912).
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

* SSF and all SSF services have been disabled (NCSDK-32000).
* Resource configuration based on UICR has been disabled (NCSDK-31999).
* The SDFW ADAC interface has been disabled (NCSDK-31994).
* SUIT is no longer supported (NCSDK-31996).

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
