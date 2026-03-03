.. _ug_production_deployment:

Production deployment best practices
####################################

.. contents::
   :local:
   :depth: 2

This guide covers several key aspects that are crucial for embedded systems development, particularly focusing on firmware management, security, and system stability.
It provides a checklist and guidance for deploying |NCS| applications to production.
Use it together with device-specific guides, :ref:`security <security_index>`, and :ref:`bootloaders and DFU <app_bootloaders>` documentation.

Before shipping firmware to production, ensure build configuration, security, updates, and reliability are aligned with your product requirements.
The following sections summarize recommended practices and point to detailed documentation.

Optimize build configuration
****************************

Using an optimize build configuration minimizes the firmware size and enhances the performance of your application.

* Build type - Prefer a release configuration (for example, using a :file:`prj_release.conf` or equivalent file suffix) so that options like optimizations and logging are set for production.
  See :ref:`modifying_build_types` and :ref:`cmake_options` for how to select configuration files and build types.
* Optimization - Enable size or speed optimizations as appropriate for your product:

  * :kconfig:option:`CONFIG_SIZE_OPTIMIZATIONS` - Reduces flash and RAM usage (recommended when size is critical).
  * :kconfig:option:`CONFIG_SPEED_OPTIMIZATIONS` - Favors execution speed.
    Use when performance needs to be improved than reducing the size.

* Boot banner - Disable in production to reduce clutter and size.
  Set :kconfig:option:`CONFIG_BOOT_BANNER` and :kconfig:option:`CONFIG_NCS_BOOT_BANNER` to ``n``.
* Remove Development Tools - Remove debugging tools and symbols in the production building to reduce the firmware size and protect intellectual property.

See :ref:`configuration_and_build` for more information on build configurations.

Minimize Debugging and Logging features
***************************************

You can reduce the overhead and prevent exposure of sensitive information by performing the following actions:

* Log level - Set :kconfig:option:`CONFIG_LOG_DEFAULT_LEVEL` to a low value (for example ``0`` for off, or ``1`` or ``2`` for errors or warnings only) in your release configuration.
  Avoid ``LOG_LEVEL_DBG`` (``4``) in production.
* Logging subsystem - For minimal footprint, you can disable the logging subsystem entirely in release builds with :kconfig:option:`CONFIG_LOG` set to ``n``, if your application does not require any runtime logs.
* Console and serial - Disable console and UART console in production so that logs and shell commands are not exposed:

  Set :kconfig:option:`CONFIG_SERIAL`, :kconfig:option:`CONFIG_CONSOLE`, and :kconfig:option:`CONFIG_UART_CONSOLE` to ``n`` if not needed.
  See also :ref:`ug_matter_device_security` (disable debug serial port).

* Assertions - In release builds, set :kconfig:option:`CONFIG_ASSERT` to ``n`` unless you have a specific need for assertions in the field.

See :ref:`debugging` and :ref:`ug_logging` for more details on Debugging and logging, respectively.

DFU and FOTA
************

You can enable secure and reliable firmware updates over-the-air in the following ways:

* Bootloader - Use :ref:`MCUboot with nRF Secure Immutable Bootloader (NSIB) <ug_bootloader_mcuboot_nsib>` (or the appropriate bootloader for your SoC) and choose an MCUboot mode that supports updates (for example, swap, overwrite, or direct-XIP with revert).
  See :ref:`ug_bootloader_main_config` for modes and :ref:`app_bootloaders`.
* Update path - Integrate and test the full update path (download, install, swap or activate, rollback if supported) for each image you update (For example, application, network core, modem).
  Use the :ref:`DFU samples <dfu_samples>` and device-specific FOTA guides (for example, :ref:`ug_nrf53_developing_ble_fota`, :ref:`ug_nrf52_developing_ble_fota`, :ref:`ug_nrf54l_developing_ble_fota`) as reference.
* Serial recovery - :kconfig:option:`CONFIG_MCUBOOT_SERIAL` enables serial recovery.
  Consider whether to enable it in production (for example, for service or recovery only) and protect or disable it when not needed.

See :ref:`app_bootloaders` for more information.

Signed images (MCUboot)
=======================

Always use cryptographically signed images in production.
Do not rely on unsigned or integrity-only schemes for field devices:

* Signature type - Configure MCUboot to use a strong signature type:

  * :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256`, or
  * :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519`, or
  * :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA` (for example, 2048/3072 bits).

  See :ref:`ug_bootloader_main_config` (Signature types) and sysbuild options in :ref:`sysbuild_forced_options`.

* Do not use :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE` in production.
  It does not provide authenticity.

* Key storage - On devices that support it, consider storing the public key in hardware (for example, :kconfig:option:`SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU` on nRF54L) for stronger protection.

Firmware versioning
*******************

Maintain a clear, consistent firmware version for each image.
See :ref:`ug_versioning_in_matter` for more information.

Graceful recovery from errors
*****************************

Ensure the system can recover from critical errors without user intervention using the following methods:

* Reset on fatal error - The |NCS| :ref:`fatal error handler <lib_fatal_error>` can reboot the device on a fatal error.
  For embedded targets, the default is to reboot.
  Ensure :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR` is set to ``y`` in production unless you have a reason to keep the device in an endless loop (for example, for diagnostics).
  This allows the device to recover from crashes and assertion failures.

* Watchdog timer - Use the Zephyr watchdog API (or a product-specific wrapper) to detect hung tasks and trigger a reset.
  Configure a timeout that fits your application (for example, main loop, communication threads).
  For an example in Matter samples, see :ref:`ug_matter_device_watchdog`, the same patterns can be applied in non-Matter applications.

Security Enhancements
*********************

Protect your devices from unauthorized debug access:

* AP-Protect - Enable the access port protection mechanism so that debuggers cannot read or write CPU and memory.
  See :ref:`app_approtect`.
  For production devices, It is recommended to enable AP-Protect (for example, by writing ``Enabled`` to ``UICR.APPROTECT`` where applicable) and following the device-specific procedure.
  Disabling it typically requires a full erase.

* Key and data protection - Enabling AP-Protect also helps protect keys and sensitive data from extraction through debug interfaces.
  See :ref:`key_storage` (enable access port protection in production) for details.

* TrustZone and Trusted Firmware-M (TF-M) - On SoCs that implement Arm TrustZone, use Trusted Firmware-M (TF-M) with the non-secure (``/ns``) board variant to run your application in the Non-Secure Processing Environment (NSPE) and keep sensitive code and data in the Secure Processing Environment (SPE).
  See :ref:`app_boards_spe_nspe` and :ref:`ug_tfm` for architecture and :ref:`ug_tfm_index` for the full TF-M section.
  TF-M is enabled automatically when building for an ``/ns`` board.
  For production, combine with static partitions (see :ref:`ug_tfm_services`), disable TF-M debug logging in release builds, and follow :ref:`key_storage` and :ref:`secure_storage_in_ncs` for keys and data in the secure side.

Static partitions
*****************

For devices that receive FOTA, use a static partition layout so that partition positions do not change between firmware versions:

* TF-M and storag -  If you use Trusted Firmware-M (TF-M) and plan field updates, define static partitions so that TF-M storage and non-secure application regions do not move.
  See :ref:`ug_tfm_services` (note on production and static partitions).

* Partition manager - When using the partition manager, ensure your production image set is built with a fixed layout (for example, through static configuration or a fixed partition manager output) and that all future updates are compatible with that layout.

Power Management
*****************

For battery-operated or energy-conscious production devices, configure power management so that idle and sleep consumption stay within product targets.
See :ref:`app_power_opt_recommendations` for the full set of :ref:`general power optimization recommendations <app_power_opt_general>` and protocol-specific guidance.

* Disable serial and logging - Serial (UART) and verbose logging keep clocks and peripherals active and can increase idle current by hundreds of µA.
  In production, disable :kconfig:option:`CONFIG_SERIAL`, :kconfig:option:`CONFIG_CONSOLE`, and :kconfig:option:`CONFIG_UART_CONSOLE` as in the Logging and debug section.
  For builds using the ``/ns`` board variant, also set :kconfig:option:`CONFIG_TFM_LOG_LEVEL_SILENCE` to ``y`` so that TF-M does not keep the secure-side UART active.

* Device Power Management - Enable :kconfig:option:`CONFIG_PM_DEVICE` so that device drivers can suspend peripherals (for example, turn off clocks) when the CPU enters low-power states.
  This reduces idle consumption when peripherals are not in use.
  On multi-image builds (for example, nRF53 Series), ensure device PM is considered for all images where applicable.

* Disable unused peripherals and pins - Board defaults may enable peripherals or pull-ups that your application does not use.
  Disable unused peripherals in devicetree (for example, ``status = "disabled";`` for ADC, UART) and avoid strong pull-ups on I/O that are held low (for example, 10 kΩ to VDD can add hundreds of µA when the pin is driven low).
  See :ref:`app_power_opt_general` (Disable unused pins and peripherals) for details.

* External flash - If the application uses external flash only occasionally (for example, for DFU), suspend it when idle using the Device Power Management API.
  See :ref:`app_power_opt_general` (Put the external flash into sleep mode) for details.

* Unused RAM - Unused RAM still draws power.
  Power down unused RAM regions when possible using the :ref:`lib_ram_pwrdn` library to reduce consumption.

* Radio transmit power - Configure radio TX power to the minimum needed for reliable link margin, higher TX power increases range but also current.
  See :ref:`app_power_opt_general` (Configure radio transmitter power) for details.

* Protocol-specific low power - Use protocol features designed for low power in production:

  * Bluetooth Mesh - :ref:`Low Power Node (LPN) <ug_bt_mesh_configuring_lpn>` to reduce duty cycle.
  * Cellular - See :ref:`cellular_psm`.
  * Matter - :ref:`ug_matter_device_low_power_configuration`.
  * Thread - :ref:`Sleepy End Device (SED) <thread_sed_ssed>` types.
  * Wi-Fi - Enable :kconfig:option:`CONFIG_NRF_WIFI_LOW_POWER` and see :ref:`ug_nrf70_developing_powersave` for power save modes.

* System power off and application PM - For battery devices that can be turned off by the user, consider Zephyr's :ref:`zephyr:pm-guide` and, when applicable, the :ref:`caf_power_manager` (CAF Power Manager) to transition to system off or suspended states after a timeout, and to use :kconfig:option:`CONFIG_PM_DEVICE` for device suspend.
  On nRF54H, the CAF Power Manager can also imply :kconfig:option:`CONFIG_PM` and :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` for deeper integration.

* Measure and verify - Use a power profiler (for example, Power Profiler Kit II) and :ref:`app_power_opt_opp` (Online Power Profiler) to verify idle and active consumption before production release.

Certifications
**************

* Regulatory Compliance - Radio equipment must comply with regional regulations before it can be marketed or sold in a given country or economic area.
  Type approval is typically required for the end product (not the IC alone).
  Regulatory compliance for production setup involves configuring the correct regulatory domain in firmware, running the applicable tests, and ensuring no test-only or development-only radio settings are left enabled in production builds.

  * Regulatory domain - Configure the regulatory domain (country or region) so that the device operates within the allowed channels, power levels, and rules (for example, DFS, scan behavior) for each target market.
    Using the wrong domain can cause non-compliant operation and approval failures.
    For Wi-Fi (nRF70 Series), set the country of operation using :kconfig:option:`CONFIG_NRF70_REG_DOMAIN` (build time) or runtime configuration; see :ref:`ug_nrf70_developing_regulatory_support` for the regulatory database and country rules.
    For other radios (for example, Bluetooth and Thread), follow the device-specific and regional requirements in the product specification and Nordic documentation.

  * Major regional agencies - Requirements and test methods vary by region.
    Common agencies include - ETSI (Europe, CE/RED), FCC (United States), MIC (Japan), SRRC (China), and KCC (South Korea).
    Ensure your production firmware and test evidence align with the requirements of each market where you sell.

  * Wi-Fi regulatory testing - For nRF70 Series products, use the :ref:`ug_wifi_regulatory_certification` documentation for test setup, :ref:`ug_wifi_regulatory_test_cases`, antenna/band-edge compensation, and the Wi-Fi radio test sample for transmit tests.
    Configure the regulatory domain (for example, :ref:`ug_wifi_setting_regulatory_domain`) and run the applicable test cases before submitting for type approval.

  * Cellular - For cellular products (for example, nRF91 Series), modem firmware and carrier certifications already address part of the radio path.
    Ensure the end product still meets regional radio and EMC requirements and any carrier-specific obligations.
    See :ref:`lwm2m_certification` and Nordic MNO certification pages for modem or carrier aspects.
