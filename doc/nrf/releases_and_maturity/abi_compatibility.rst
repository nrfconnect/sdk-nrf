.. _abi_compatibility:

ABI compatibility
#################

.. contents::
   :local:
   :depth: 2

Application Binary Interface (ABI) compatibility defines how software components, like libraries and applications, interact at the machine code level through their low-level binary interface, including:

* Function-calling conventions
* Data structure layouts in memory
* Exception handling mechanisms
* Register usage conventions

When ABI compatibility is maintained, binaries of one component can interface correctly with another without needing recompilation.
For example, adding a new function to a library is typically an ABI-compatible change, as existing binaries remain functional.
However, changes that affect data structure layouts, such as altering field order or size, break ABI compatibility as they change the memory layout expected by existing binaries.

ABI compatibility matrix for the nRF54H20 SoC binaries
******************************************************

The following table illustrates ABI compatibility between different versions of the nRF54H20 SoC binaries and the |NCS|:

.. list-table::
   :header-rows: 1

   * - |NCS| versions
     - Compatible nRF54H20 SoC binaries version
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

ABI compatibility ensures that the Secure Domain and System Controller firmware binaries do not need to be recompiled each time the application, radio binaries, or both are recompiled, as long as they are based on a compatible |NCS| version.
Additionally, maintaining ABI compatibility allows the nRF54H20 SoC binary components to work together without recompilation when updating to newer |NCS| versions.

nRF54H20 SoC binaries v0.9.2 changelog
**************************************

.. note::
    The nRF54H20 SoC binaries only support specific versions of the |NCS| and do not support rollbacks to a previous version.
    Upgrading the nRF54H20 SoC binaries on your development kit might break the DK's compatibility with applications developed for previous versions of the |NCS|.

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
