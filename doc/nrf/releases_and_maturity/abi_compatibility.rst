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

##update links

.. list-table::
   :header-rows: 1

   * - |NCS| versions
     - Compatible nRF54H20 SoC binaries version
   * - |NCS| v3.1.0
     - `nRF54H20 SoC binaries v0.x.x`_, compatible with the nRF54H20 DK v0.9.0 and later revisions.
       It includes IRONside SE.
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

nRF54H20 SoC binaries v0.x.x changelog
**************************************

The following sections provide detailed lists of changes by component.

Secure Domain Firmware (SDFW) v21.1.1
=====================================

Added
-----

* CHIDX values are no longer required to be set when configuring ``PPIB_SUBSCRIBE_SEND`` or ``PPIB_PUBLISH_RECEIVE`` through ``PERIPHCONF``.

Fixed
-----

* An issue where the application core was booted despite the presence of boot errors.

Secure Domain Firmware (SDFW) v21.1.0
=====================================

Added
-----

* MAC in the PSA crypto service.
* Static memory checks that protect Nordic assets by whitelisting only memory ranges available to the application developers.
* System Controller Firmware (SCFW) releases are now included in the Secure Domain Firmware (SDFW).
  See :ref:`scfw_5_0_0` for details.

Updated
-------

* The CPUCONF service request definition.
* The CPU and WAIT parameters are now both packed into the first 4-byte value, and the message data is sent inline in the request.
* Support for initializing a subset of global domain peripherals by configuring ``UICR.PERIPHCONF``. This enables the initial configuration of the CTRLSEL GPIO pin, global IRQ mapping, IPC mapping, global PPIBs, and more.
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

Secure Domain Firmware (SDFW) v21.0.1
=====================================

Added
-----

* NCSDK-33583: Boot report is now written to radio core.

Updated
-------

* Enable link-time optimization.
* NCSDK-32173: Disable CRACEN microcode loading.

Fixed
-----

* NCSDK-33306: The application core is now started in halted mode when IRONside detects errors in the UICR or BICR. This ensures that it is possible to recover from these errors by writing proper values with a debugger.

Secure Domain Firmware (SDFW) v21.0.0
=====================================

Added
-----

* NCSDK-32168: Added support for the IRONside SE update service.
* NCSDK-32444: Added experimental support for a new UICR format. At this stage, the functionality is mainly for internal testing and development, and user tools for interacting with UICR will be added at a later stage.
* NCSDK-32393: Added boot report support.
* NCSDK-32925: Added CPUCONF service for booting the radio core. Only ``hello world`` is currently supported.
* NCSDK-32441: Added IRONside calls, the successor to SSF.

Updated
-------

* NCSDK-32912: The limited PSA service is now implemented as an IRONside call. This replaces the temporary IPC mechanism from the last release.

Fixed
-----

* NCSDK-33265: Fixed an issue that made it so the ``CTRLAP.BOOTSTATUS`` firmware sequence number was always set to zero.

Secure Domain Firmware (SDFW) v20.1.0
=====================================

Added
-----

* NCSDK-32163: Added experimental support for limited PSA service. This is built on top of a temporary IPC mechanism which will soon be replaced. The top-level interface will remain the same.

  The PSA support is currently limited to the following ``PSA_WANT`` symbols:

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

  To use the service, enable ``CONFIG_NRF_SECURITY=y``.

  .. note::
     The v20.1.0 support for this PSA API is not compatible with NCS 3.0.0 or NCS 3.1.0. It is only compatible with sdk-nrf revision ``1b2abb07b8b2528ecaf86f54e0c6cf48c425055a``.

     When updating to NCS 3.1.0 you must simultaneously update to a compatible IRONside SE.

Updated
-------

* ``EXTRAVERSION`` is now included in ``SICR.TBS.x.VERSION``.

Secure Domain Firmware (SDFW) v20.0.1
=====================================

Updated
-------

* AUX-AP is now always disabled.
* Internal optimization of MRAM and RAM usage.

Removed
-------

* Removed initializing TDD on system boot as it is not retained.
* Removed initializing P6 and P7 pins to be EXMIF/TPIU to prevent unacceptable output states.

Secure Domain Firmware (SDFW) v20.0.0
=====================================

This release is the first release that is based on the new Secure Domain firmware architecture. Most of the functionality from the preceding version of SDFW has been disabled or removed and will be gradually reintroduced in upcoming versions.

Added
-----

* NCSDK-31997: The ERASEALL command is now supported through the boot command interface.
* The DEBUGWAIT command is now supported through the boot command interface.
* NCSDK-32355: A new scheme for status reporting through the ``BOOTSTATUS`` register in CTRL-AP has been added.

Updated
-------

* NCSDK-31993: SCFW is now included in the URoT firmware partition, and the SysCtrl CPU is always started.
* NCSDK-31995: SDFW no longer starts the Radio core. The Application core is now always started with the secure VTOR set to the first address following the Secure Domain firmware partitions.
* NCSDK-31999: SDFW now statically configures the device at boot so that most resources are accessible by the Application core without needing to modify the UICR.

Removed (from legacy SUIT-based SDFW)
-------------------------------------

* NCSDK-32000: SSF and all SSF services have been disabled.
* NCSDK-31999: Resource configuration based on UICR has been disabled.
* NCSDK-31994: The SDFW ADAC interface has been disabled.
* NCSDK-31996: SUIT is no longer supported.

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
