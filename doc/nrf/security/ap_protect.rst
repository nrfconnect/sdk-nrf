.. _app_approtect:

Managing access port protection
###############################

.. contents::
   :local:
   :depth: 2

.. app_approtect_info_start

Several Nordic Semiconductor SoCs or SiPs supported in the |NCS| offer an implementation of the access port protection mechanism (AP-Protect).
When enabled, this mechanism blocks the debugger from read and write access to all CPU registers and memory-mapped addresses.
Accessing these registers and addresses again requires disabling the mechanism and erasing the flash.

.. app_approtect_info_end

.. _app_approtect_general_reference:
.. _app_approtect_implementation_overview:

Implementation overview
***********************

Nordic Semiconductor devices implement access port protection using the following mechanisms:

Hardware AP-Protect
   * Protection is controlled solely by the ``UICR.APPROTECT`` register.
   * Devices ship with AP-Protect disabled (debug access open).
   * Used on nRF9160 and older HW build codes of nRF52 Series devices.
   * Additionally, nRF54H20 devices support hardware AP-Protect through the :ref:`IronSide SE <ug_nrf54h20_ironside>`.

Hardware and software AP-Protect
   * Protection is controlled by both ``UICR.APPROTECT`` and software.
   * Devices ship with AP-Protect enabled (debug access blocked) and it re-enables on every reset.
   * Used on nRF53, nRF54L, nRF91x1, and newer HW build codes of nRF52 Series devices.

Secure AP-Protect
   * An additional protection layer for SoCs or SiPs that support `ARM TrustZone`_ and different :ref:`app_boards_spe_nspe`.
   * When enabled, it blocks access only to the Secure Processing Environment (SPE), while allowing non-secure debugging.
   * Works alongside standard AP-Protect.
   * Available on nRF5340, nRF54L, and nRF91 Series devices.

The following diagram illustrates the relationship between the implementation types:

.. code-block:: none

   ┌───────────────────────────────────┐
   │       Hardware AP-Protect         │    nRF9160, older nRF52
   └───────────────────────────────────┘
                    │
                    │ Evolution
                    ▼
   ┌───────────────────────────────────┐
   │  Hardware + Software AP-Protect   │    nRF53, nRF54L, nRF91x1, newer nRF52
   └───────────────────────────────────┘
                    │
                    │ + TrustZone
                    │
   ┌───────────────────────────────────┐
   │       Secure AP-Protect           │    nRF5340, nRF54L, nRF91 Series
   └───────────────────────────────────┘

See the following sections for more information about the available implementation types.

.. note::
   Some devices also support ``UICR.ERASEPROTECT``, which prevents the ``ERASEALL`` command from executing and stops the device from being erased.
   If both AP-Protect and ``UICR.ERASEPROTECT`` are enabled, the device cannot be unlocked or recovered.
   See the hardware documentation for your specific device for details about ``UICR.ERASEPROTECT`` availability and configuration.

Hardware AP-Protect flow
========================

This flow applies to the nRF9160 and older HW build codes of nRF52 Series devices.

By default, the AP-Protect is disabled.
To enable it, write ``Enabled`` to ``UICR.APPROTECT`` and reset the device.
To disable it, make sure ``UICR.ERASEPROTECT`` is disabled (if needed for your device) and issue an ``ERASEALL`` command using CTRL-AP.
This command erases the flash, RAM, and UICR (including ``UICR.APPROTECT``), and resets the device.

.. msc::

   hscale = "1.5";
   Debugger, "CTRL-AP", Device, Flash;

   Debugger->"CTRL-AP"     [label="Connect"];
   "CTRL-AP"--Device       [label="Access open (AP-Protect disabled by default)"];
   ...;
   --- [label="To enable AP-Protect:"];
   Debugger->"CTRL-AP"     [label="Program UICR.APPROTECT to enabled state"];
   "CTRL-AP"--Device       [label="Access blocked (AP-Protect enabled)"];
   ...;
   --- [label="To disable AP-Protect:"];
   Debugger->"CTRL-AP"     [label="Program UICR.ERASEPROTECT to disabled state (if needed)"];
   Debugger->"CTRL-AP"     [label="Issue ERASEALL"];
   "CTRL-AP"->Flash        [label="Erase Flash, RAM, UICR (incl. UICR.APPROTECT)"];
   "CTRL-AP"->Device       [label="Reset"];
   Device->"CTRL-AP"       [label="AP-Protect disabled"];
   "CTRL-AP"->Debugger     [label="Full debug access granted"];

Hardware and software AP-Protect flow
=====================================

This flow applies to nRF53, nRF54L, nRF91x1, and newer HW build codes of nRF52 Series devices.

By default, the AP-Protect is enabled.
To disable it, issue an ``ERASEALL`` command and make sure to meet the following conditions to keep the device unlocked:

* ``UICR.ERASEPROTECT`` is programmed to a disabled state (if needed for your device).
* ``UICR.APPROTECT`` is programmed to a disabled state.
* Firmware disables AP-Protect in software during boot.

To enable it, write ``Enabled`` to ``UICR.APPROTECT`` and reset the device.

.. msc::
    hscale = "1.5";
    Debugger, "CTRL-AP", Device, Firmware, UICR;

    Debugger->"CTRL-AP"     [label="Connect"];
    "CTRL-AP"--Device       [label="Access blocked (AP-Protect enabled by default)"];
    ...;
    --- [label="To disable AP-Protect:"];
    Debugger->"CTRL-AP"     [label="Issue ERASEALL"];
    "CTRL-AP"->Device       [label="Erase Flash, RAM, UICR (incl. UICR.APPROTECT)"];
    "CTRL-AP"->Device       [label="Reset"];
    ...;
    --- [label="To keep AP-Protect disabled after reset, conditions must be met:"];
    ...;
    Debugger box UICR       [label="Program UICR.ERASEPROTECT to disabled state (if needed for your device)"];
    Debugger->"CTRL-AP"     [label="Program UICR.ERASEPROTECT"];
    "CTRL-AP"->UICR         [label="Write disabled state to ERASEPROTECT.DISABLE"];
    ...;
    Debugger box UICR       [label="Program UICR.APPROTECT to disabled state (HwDisabled, HwUnprotected, Unprotected, depending on the device)"];
    Debugger->"CTRL-AP"     [label="Program UICR.APPROTECT"];
    "CTRL-AP"->UICR         [label="Write disabled state to APPROTECT.DISABLE"];
    ...;
    Debugger box UICR       [label="Firmware must disable AP-Protect in software during boot"];
    Debugger->"CTRL-AP"     [label="Flash firmware"];
    Device box Firmware     [label="Firmware runs SystemInit()"];
    Firmware->Device        [label="Disable AP-Protect in software"];
    Device->Debugger        [label="Debug access remains open"];

.. _secure_approtect_support:

Secure AP-Protect flow
======================

This flow applies to TrustZone-enabled devices (nRF5340, nRF54L, nRF91 Series) when Secure AP-Protect is enabled.
Such devices use :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` and :ref:`security by separation <app_boards_spe_nspe>`, where a Secure Processing Environment (SPE) that is isolated from the Non-Secure Processing Environment (NSPE).

While AP-Protect blocks access to all CPU registers and memories, Secure AP-Protect limits the CPU access to the NSPE side only.
This allows debugging of the NSPE, while the SPE debugging is blocked.

Secure AP-Protect works alongside standard AP-Protect:

- AP-Protect blocks access to all CPU registers and memories.
- Secure AP-Protect limits access to the CPU to only NSPE access.
  This means that the CPU is entirely unavailable while it is running the code in the SPE, and only non-secure registers and address-mapped resources can be accessed.

By default, the Secure AP-Protect is enabled.
To disable it, issue an ``ERASEALL`` command and make sure to meet the following conditions to keep the device unlocked:

* ``UICR.ERASEPROTECT`` is programmed to a disabled state (if needed for your device).
* ``UICR.APPROTECT`` is programmed to a disabled state.
* Firmware disables AP-Protect in software during boot.

To enable it, write ``Enabled`` to ``UICR.APPROTECT`` and reset the device.

For information about how to configure Secure AP-Protect in the |NCS|, see :ref:`app_secure_approtect`.

.. msc::
    hscale = "1.5";
    Debugger, "CTRL-AP", Device, NSPE, SPE;

    Debugger->"CTRL-AP"     [label="Connect"];
    "CTRL-AP"--SPE          [label="SPE access blocked (Secure AP-Protect enabled by default)"];
    "CTRL-AP"->NSPE         [label="NSPE access allowed"];
    Debugger->"CTRL-AP"     [label="Debug NSPE code"];
    "CTRL-AP"->NSPE         [label="Read/write non-secure memory"];
    ...;
    --- [label="To unlock SPE debugging:"];
    Debugger->"CTRL-AP"     [label="Issue ERASEALL"];
    "CTRL-AP"->Device       [label="Erase Flash, UICR, RAM (incl. UICR.APPROTECT)"];
    "CTRL-AP"->Device       [label="Reset"];
    Debugger->"CTRL-AP"     [label="Configure UICR.SECUREAPPROTECT"];
    Device->Debugger        [label="Full SPE + NSPE debug access"];
    ...;
    --- [label="To keep Secure AP-Protect disabled after reset, conditions must be met:"];
    ...;
    Debugger box SPE       [label="Program UICR.ERASEPROTECT to disabled state (if needed for your device)"];
    Debugger->"CTRL-AP"     [label="Program UICR.ERASEPROTECT"];
    "CTRL-AP"->SPE         [label="Write disabled state to ERASEPROTECT.DISABLE"];
    ...;
    Debugger box SPE        [label="Program UICR.SECUREAPPROTECT to disabled state (HwDisabled, HwUnprotected, Unprotected, depending on the device)"];
    Debugger->"CTRL-AP"     [label="Program UICR.SECUREAPPROTECT"];
    "CTRL-AP"->SPE          [label="Write disabled state to SECUREAPPROTECT.DISABLE"];
    ...;
    Debugger box SPE        [label="Firmware must disable Secure AP-Protect in software during boot"];
    Debugger->"CTRL-AP"     [label="Flash firmware"];
    Device box SPE          [label="Firmware runs SystemInit()"];
    SPE->Device             [label="Disable Secure AP-Protect in software"];
    Device->Debugger        [label="Debug access remains open"];

.. _app_approtect_ncs:
.. _app_approtect_ncs_lock:
.. _app_approtect_ncs_user_handling:
.. _app_approtect_ncs_use_uicr:
.. _app_approtect_uicr_approtect:
.. _app_secure_approtect:
.. _app_secure_approtect_uicr_approtect:
.. _app_approtect_device_series:

Configuring AP-Protect per device
*********************************

The following sections provide device-specific information about AP-Protect configuration.

.. note::
   When the table mentions "depending on HW build code", it means that the AP-Protect support is different depending on the build code of the device.
   Check the hardware documentation for the build code differences.

.. _app_approtect_nrf91_series:

nRF91 Series
============

.. tabs::

   .. tab:: nRF9160

      .. list-table:: nRF9160 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✔
           - ✗
           - ✔
           - `Debugger access protection for nRF9160`_

      On the nRF9160, AP-Protect and Secure AP-Protect are *hardware-only*; there are no |NCS| Kconfig options for this device.
      Both mechanisms are controlled solely by writing to the UICR using nRF Util.
      For more information about the ``nrfutil device protection-set`` command, see `Configuring readback protection`_ in the nRF Util documentation.

      .. note::
         This device supports ``UICR.ERASEPROTECT``, which might prevent the ``ERASEALL`` command from executing when AP-Protect is enabled.
         See hardware documentation for more information.

      **Enabling AP-Protect:**

      To enable hardware AP-Protect, write ``Enabled`` to ``UICR.APPROTECT`` using the following nRF Util command:

      .. code-block:: console

         nrfutil device protection-set All

      This command enables the hardware AP-Protect and resets the device.

      **Enabling Secure AP-Protect:**

      To enable hardware Secure AP-Protect, use the following nRF Util command:

      .. code-block:: console

         nrfutil device protection-set SecureRegions

      This command enables the hardware Secure AP-Protect and resets the device.

      **Disabling AP-Protect:**

      To disable hardware AP-Protect, issue an ``ERASEALL`` command using the following nRF Util command:

      .. code-block:: console

         nrfutil device recover

      The device is automatically unlocked after erase.
      This command can also disable ``UICR.ERASEPROTECT``.
      No changes in firmware are required because the nRF9160 does not use software AP-Protect.

      **Production programming:**

      For the devices that are in a production environment, it is highly recommended to lock the ``UICR.APPROTECT`` register to prevent unauthorized access to the device.
      If the access port protection is configured this way, it cannot be disabled without erasing the flash memory.

      Check also the following documentation pages for more information:

      * `Unlocking nRF91`_
      * `Enabling device protection on nRF91`_
      * `Checking AP-Protect status on nRF91`_

   .. tab:: nRF9161

      .. list-table:: nRF9161 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✗
           - ✔
           - ✔
           - `AP-Protect for nRF9161`_

      .. nrf9161_approtect_tab_start

      The following Kconfig options configure AP-Protect and Secure AP-Protect on nRF91x1 devices in the |NCS|.

      .. list-table:: nRF91x1 AP-Protect configuration options in the |NCS|
         :header-rows: 1
         :align: center
         :widths: auto

         * - Desired AP-Protect state
           - Kconfig option
           - Description
         * - Enabled
           - :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK`
           - | With this Kconfig option set, the MDK locks AP-Protect in ``SystemInit()`` at every boot. UICR is not modified by this Kconfig option.
             |
             | **Note:** The register setting does not persist in System ON IDLE mode. You must lock the ``UICR.APPROTECT`` register using nRF Util (see Enabling AP-Protect below).
             |
             | For multi-image boot, this option needs to be set in *the first image* (like a secure bootloader). Otherwise, the software AP-Protect will be opened for subsequent images.
             | You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_APPROTECT_LOCK` Kconfig option to set it for all images at once.
         * - Authenticated
           - :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING`
           - | With this Kconfig option set, AP-Protect is left enabled and it is up to the user-space code to handle unlocking the device if needed.
             | The MDK will close the debug AHB-AP, but not lock it, so the AHB-AP can be reopened by the firmware.
             | Reopening the AHB-AP must be preceded by a handshake operation over UART, CTRL-AP Mailboxes, or some other communication channel.
             |
             | You can use this Kconfig option for example to implement the authenticated debug and lock. See the SoC or SiP hardware documentation for more information.
             |
             | For multi-image boot, this option needs to be set for *all images*. The default value is to open the device. This allows the debugger to be attached to the device.
             | You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_APPROTECT_USER_HANDLING` Kconfig option to set it for all images at once.
         * - Disabled
           - :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR`
           - | With this Kconfig option set, AP-Protect follows the UICR register. If UICR is open (``UICR.APPROTECT`` disabled), AP-Protect is disabled. This is the default in the |NCS|.
             | You can start debugging the firmware without additional steps needed.

      .. list-table:: nRF91x1 Secure AP-Protect configuration options in the |NCS|
         :header-rows: 1
         :align: center
         :widths: auto

         * - Desired Secure AP-Protect state
           - Kconfig option or method
           - Description
         * - Enabled
           - Locking ``UICR.SECUREAPPROTECT`` with nRF Util
           - nRF91x1 devices do not support :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK`. To enable hardware Secure AP-Protect, you must lock the ``UICR.SECUREAPPROTECT`` register using nRF Util (see below).
         * - Authenticated
           - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING`
           - | With this Kconfig option set, Secure AP-Protect is left enabled and you can handle its state at a later stage.
             | You can use this option for example to implement the authenticated debug and lock. See the SoC or SiP hardware documentation for more information.
             |
             | For multi-image boot, this option needs to be set for *all images*. The default value is to open the device. This allows the debugger to be attached to the device.
             | You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_SECURE_APPROTECT_USER_HANDLING` Kconfig option to set it for all images at once.
         * - Disabled
           - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR`
           - | With this Kconfig option set, Secure AP-Protect follows the UICR register. If UICR is open (``UICR.APPROTECT`` disabled), Secure AP-Protect is disabled. This is the default in the |NCS| for the nRF91x1 devices.
             | You can start debugging the SPE without additional steps needed.

      .. note::
         This device supports ``UICR.ERASEPROTECT``, which might prevent the ``ERASEALL`` command from executing when AP-Protect is enabled.
         See hardware documentation for more information.

      **Enabling AP-Protect:**

      On nRF91x1, using the Kconfig option :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` is not supported because the setting does not persist in System ON IDLE mode.
      To enable hardware AP-Protect, write ``Enabled`` to ``UICR.APPROTECT`` using the following nRF Util command:

      .. code-block:: console

         nrfutil device protection-set All

      This command enables the hardware AP-Protect and resets the device.

      **Enabling Secure AP-Protect:**

      To enable hardware Secure AP-Protect:

      .. code-block:: console

         nrfutil device protection-set SecureRegions

      This command enables the hardware Secure AP-Protect and resets the device.

      For more information about the ``nrfutil device protection-set`` command, see `Configuring readback protection`_ in the nRF Util documentation.

      .. note::
         With devices that use software AP-Protect, nRF Util cannot enable hardware Secure AP-Protect if the software Secure AP-Protect is already enabled.
         If you encounter errors with nRF Util, make sure that software AP-Protect and software Secure AP-Protect are disabled.

      **Disabling AP-Protect:**

      If you want to keep the AP-Protect disabled after reset, you must flash firmware that writes ``SwDisable`` to ``APPROTECT.DISABLE`` during boot (``SwDisable`` to ``SECUREAPPROTECT.DISABLE`` during boot for Secure AP-Protect).
      In the |NCS|, :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR` and :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR` (enabled by default) handle the software unlock.

      To disable AP-Protect, issue an ``ERASEALL`` command using the following nRF Util command:

      .. code-block:: console

         nrfutil device recover

      The device is automatically unlocked after erase.
      This command can also disable ``UICR.ERASEPROTECT``.

      **Production programming:**

      For the devices that are in a production environment, it is highly recommended to lock the ``UICR.APPROTECT`` register to prevent unauthorized access to the device. If the access port protection is configured this way, it cannot be disabled without erasing the flash memory.

      Check also the following documentation pages for more information:

      * `Unlocking nRF91`_
      * `Enabling device protection on nRF91`_
      * `Checking AP-Protect status on nRF91`_

      .. nrf9161_approtect_tab_end

   .. tab:: nRF9151

      .. list-table:: nRF9151 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✗
           - ✔
           - ✔
           - `AP-Protect for nRF9151`_

      The following Kconfig options configure AP-Protect on the nRF9151 in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf9161_approtect_tab_start
         :end-before: .. nrf9161_approtect_tab_end

   .. tab:: nRF9131

      .. list-table:: nRF9131 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✗
           - ✔
           - ✔
           - *Documentation not yet available*

      The following Kconfig options configure AP-Protect on the nRF9131 in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf9161_approtect_tab_start
         :end-before: .. nrf9161_approtect_tab_end


.. _app_approtect_nrf54l_series:

nRF54L Series
=============

.. tabs::

   .. tab:: nRF54L15

      .. list-table:: nRF54L15 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✗
           - ✔
           - ✔
           - `AP-Protect for nRF54L15`_

      .. nrf54l15_approtect_tab_start

      The following Kconfig options configure AP-Protect and Secure AP-Protect on the nRF54L15 in the |NCS|:

      .. list-table:: AP-Protect configuration options in the |NCS|
         :header-rows: 1
         :align: center
         :widths: auto

         * - Desired AP-Protect state
           - Kconfig option
           - Description
         * - Enabled
           - :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK`
           - | With this Kconfig option set, the MDK locks AP-Protect in ``SystemInit()`` at every boot. UICR is not modified by this Kconfig option.
             |
             | For multi-image boot, this option needs to be set in *the first image* (like a secure bootloader). Otherwise, the software AP-Protect will be opened for subsequent images.
             | You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_APPROTECT_LOCK` Kconfig option to set it for all images at once.
         * - Authenticated
           - :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING`
           - | With this Kconfig option set, AP-Protect is left enabled and it is up to the user-space code to handle unlocking the device if needed.
             | The MDK will close the debug AHB-AP, but not lock it, so the AHB-AP can be reopened by the firmware.
             | Reopening the AHB-AP must be preceded by a handshake operation over UART, CTRL-AP Mailboxes, or some other communication channel.
             |
             | You can use this Kconfig option for example to implement the authenticated debug and lock. See the SoC or SiP hardware documentation for more information.
             |
             | For multi-image boot, this option needs to be set for *all images*. The default value is to open the device. This allows the debugger to be attached to the device.
             | You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_APPROTECT_USER_HANDLING` Kconfig option to set it for all images at once.
         * - Disabled
           - :kconfig:option:`CONFIG_NRF_APPROTECT_DISABLE`
           - | With this Kconfig option set, AP-Protect is disabled. This is the default in the |NCS| for this device family.
             | You can start debugging the firmware without additional steps needed.

      .. list-table:: Secure AP-Protect configuration options in the |NCS|
         :header-rows: 1
         :align: center
         :widths: auto

         * - Desired Secure AP-Protect state
           - Kconfig option
           - Description
         * - Enabled
           - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK`
           - | With this Kconfig option set, the MDK locks Secure AP-Protect in ``SystemInit()`` at every boot.
             | For hardware protection, the ``UICR.SECUREAPPROTECT`` register must be written using the nRF Util command (see below).
             |
             | For multi-image boot, this option needs to be set in *the first image* (like a secure bootloader). Otherwise, the software Secure AP-Protect will be opened for subsequent images.
             | You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_SECURE_APPROTECT_LOCK` Kconfig option to set it for all images at once.
         * - Authenticated
           - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING`
           - | With this Kconfig option set, Secure AP-Protect is left enabled and it is up to the user-space code to handle unlocking the device if needed.
             | You can use this option for example to implement the authenticated debug and lock. See the SoC or SiP hardware documentation for more information.
             |
             | For multi-image boot, this option needs to be set for *all images*. The default value is to open the device. This allows the debugger to be attached to the device.
             | You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_SECURE_APPROTECT_USER_HANDLING` Kconfig option to set it for all images at once.
         * - Disabled
           - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_DISABLE`
           - Secure AP-Protect is disabled. This is the default in the |NCS| for this device family.

      .. note::
         This device supports ``UICR.ERASEPROTECT``, which might prevent the ``ERASEALL`` command from executing when AP-Protect is enabled.
         See hardware documentation for more information.

      **Enabling AP-Protect:**

      To enable hardware AP-Protect, write ``Enabled`` to ``UICR.APPROTECT`` using the following nRF Util command:

      .. code-block:: console

         nrfutil device protection-set All

      This set of commands enables the hardware AP-Protect and resets the device.

      **Enabling Secure AP-Protect:**

      To enable hardware Secure AP-Protect, write ``Enabled`` to ``UICR.SECUREAPPROTECT`` using the following nRF Util command:

      .. code-block:: console

         nrfutil device protection-set SecureRegions

      This command enables hardware Secure AP-Protect and resets the device.

      For more information about the ``nrfutil device protection-set`` command, see `Configuring readback protection`_ in the nRF Util documentation.

      .. note::
         With devices that use software AP-Protect, nRF Util cannot enable hardware Secure AP-Protect if the software Secure AP-Protect is already enabled.
         If you encounter errors with nRF Util, make sure that software AP-Protect and software Secure AP-Protect are disabled.

      **Disabling AP-Protect:**

      If you want to keep the AP-Protect disabled after reset, you must flash firmware that opens the `debugger signals in Tamper Controller <nRF54L15 Debugger signals_>`_.
      In the |NCS|, :kconfig:option:`CONFIG_NRF_APPROTECT_DISABLE` and :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_DISABLE` (enabled by default) handle the software unlock.

      To disable AP-Protect, issue an ``ERASEALL`` command using the following nRF Util command:

      .. code-block:: console

         nrfutil device recover

      The device is automatically unlocked after erase.
      This command can also disable ``UICR.ERASEPROTECT``.

      **Production programming:**

      For the devices that are in a production environment, it is highly recommended to lock the ``UICR.APPROTECT`` register to prevent unauthorized access to the device. If the access port protection is configured this way, it cannot be disabled without erasing the flash memory.

      Check also the following documentation pages for more information:

      * `Disabling AP-Protect on nRF54L`_
      * `Enabling device protection on nRF54L`_
      * `Checking AP-Protect status on nRF54L`_

      .. nrf54l15_approtect_tab_end

   .. tab:: nRF54L10

      .. list-table:: nRF54L10 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✗
           - ✔
           - ✔
           - `AP-Protect for nRF54L10 <AP-Protect for nRF54L15_>`_

      The following Kconfig options configure AP-Protect on the nRF54L10 in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf54l15_approtect_tab_start
         :end-before: .. nrf54l15_approtect_tab_end

   .. tab:: nRF54L05

      .. list-table:: nRF54L05 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✗
           - ✔
           - ✔
           - `AP-Protect for nRF54L05 <AP-Protect for nRF54L15_>`_

      The following Kconfig options configure AP-Protect on the nRF54L05 in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf54l15_approtect_tab_start
         :end-before: .. nrf54l15_approtect_tab_end

   .. tab:: nRF54LM20A

      .. list-table:: nRF54LM20A AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✗
           - ✔
           - ✔
           - `AP-Protect for nRF54LM20A`_

      The following Kconfig options configure AP-Protect on the nRF54LM20A in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf54l15_approtect_tab_start
         :end-before: .. nrf54l15_approtect_tab_end

   .. tab:: nRF54LV10A

      .. list-table:: nRF54LV10A AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✗
           - ✔
           - ✔
           - `AP-Protect for nRF54LV10A`_

      The following Kconfig options configure AP-Protect on the nRF54LV10A in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf54l15_approtect_tab_start
         :end-before: .. nrf54l15_approtect_tab_end

.. _app_approtect_nrf54h_series:

nRF54H Series
=============

.. tabs::

   .. tab:: nRF54H20

      .. list-table:: nRF54H20 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect (through IronSide SE)
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✔
           - ✗
           - ✗
           - n/a

      On nRF54H20, AP-Protect is not controlled by the standard |NCS| Kconfig options.
      It is managed through :ref:`IronSide SE <ug_nrf54h20_ironside>`.
      See the :ref:`nRF54H20-specific UICR.APPROTECT <ug_nrf54h20_ironside_se_uicr_approtect>` documentation for how to configure AP-Protect on this device.

      .. note::
         This device supports ``UICR.ERASEPROTECT``, which might prevent the ``ERASEALL`` command from executing when AP-Protect is enabled.

      **Enabling AP-Protect:**

      To enable hardware AP-Protect, write ``Enabled`` to ``UICR.APPROTECT`` using IronSide SE and reset the device.
      For configuration details, see the :ref:`nRF54H20-specific UICR.APPROTECT <ug_nrf54h20_ironside_se_uicr_approtect>` documentation.

      **Disabling AP-Protect:**

      To disable hardware AP-Protect, issue an ``ERASEALL`` command using the following nRF Util command:

      .. code-block:: console

         nrfutil device recover

      The device is automatically unlocked after erase.
      This command can also disable ``UICR.ERASEPROTECT``.

.. _app_approtect_nrf53_series:

nRF53 Series
============

.. tabs::

   .. tab:: nRF5340

      .. list-table:: nRF5340 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✗
           - ✔
           - ✔
           - `AP-Protect for nRF5340`_

      The following Kconfig options configure AP-Protect on the nRF5340 in the |NCS|:

      .. list-table:: nRF5340 AP-Protect configuration options in the |NCS|
         :header-rows: 1
         :align: center
         :widths: auto

         * - Desired AP-Protect state
           - Kconfig option
           - Description
         * - Enabled
           - :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK`
           - | With this Kconfig option set, the MDK locks AP-Protect in ``SystemInit()`` at every boot. UICR is not modified by this Kconfig option.
             |
             | For multi-image boot, this option needs to be set in *the first image* (like a secure bootloader). Otherwise, the software AP-Protect will be opened for subsequent images.
             | You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_APPROTECT_LOCK` Kconfig option to set it for all images at once.
         * - Authenticated
           - :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING`
           - | With this Kconfig option set, AP-Protect is left enabled and it is up to the user-space code to handle unlocking the device if needed.
             | The MDK will close the debug AHB-AP, but not lock it, so the AHB-AP can be reopened by the firmware.
             | Reopening the AHB-AP must be preceded by a handshake operation over UART, CTRL-AP Mailboxes, or some other communication channel.
             |
             | You can use this Kconfig option for example to implement the authenticated debug and lock. See the SoC or SiP hardware documentation for more information.
             |
             | For multi-image boot, this option needs to be set for *all images*. The default value is to open the device. This allows the debugger to be attached to the device.
             | You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_APPROTECT_USER_HANDLING` Kconfig option to set it for all images at once.
         * - Disabled
           - :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR`
           - | With this Kconfig option set, AP-Protect follows the UICR register. If UICR is open (``UICR.APPROTECT`` disabled), AP-Protect is disabled. This is the default in the |NCS|.
             | You can start debugging the firmware without additional steps needed.

      The following Kconfig options configure Secure AP-Protect on the nRF5340 in the |NCS| when you are using TF-M:

      .. list-table:: nRF5340 Secure AP-Protect configuration options in the |NCS|
         :header-rows: 1
         :align: center
         :widths: auto

         * - Desired Secure AP-Protect state
           - Kconfig option
           - Description
         * - Enabled
           - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK`
           - | With this Kconfig option set, the MDK locks Secure AP-Protect in ``SystemInit()`` at every boot.
             | For hardware protection, the ``UICR.SECUREAPPROTECT`` register must be written using the nRF Util command (see below).
             |
             | For multi-image boot, this option needs to be set in *the first image* (like a secure bootloader). Otherwise, the software Secure AP-Protect will be opened for subsequent images.
             | You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_SECURE_APPROTECT_LOCK` Kconfig option to set it for all images at once.
         * - Authenticated
           - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING`
           - | With this Kconfig option set, Secure AP-Protect is left enabled and it is up to the user-space code to handle unlocking the device if needed, for example for authenticated debugging of the SPE.
             |
             | For multi-image boot, this option needs to be set for *all images*. The default value is to open the device. This allows the debugger to be attached to the device.
             | You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_SECURE_APPROTECT_USER_HANDLING` Kconfig option to set it for all images at once.
         * - Disabled
           - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR`
           - | With this Kconfig option set, Secure AP-Protect follows the UICR register. If UICR is open (``UICR.SECUREAPPROTECT`` disabled), Secure AP-Protect is disabled. This is the default in the |NCS|.
             | You can start debugging the SPE without additional steps needed.

      .. note::
         This device supports ``UICR.ERASEPROTECT``, which might prevent the ``ERASEALL`` command from executing when AP-Protect is enabled.
         See hardware documentation for more information.

      **Enabling AP-Protect:**

      To enable hardware AP-Protect on both cores, write ``Enabled`` to ``UICR.APPROTECT`` using the following nRF Util command:

      .. code-block:: console

         nrfutil device protection-set All --core Network
         nrfutil device protection-set All

      The order of the commands is important.
      This set of commands enables the hardware AP-Protect (and Secure AP-Protect) and resets the device.

      **Enabling Secure AP-Protect:**

      To enable hardware Secure AP-Protect, use the following nRF Util command:

      .. code-block:: console

         nrfutil device protection-set SecureRegions

      For more information about the ``nrfutil device protection-set`` command, see `Configuring readback protection`_ in the nRF Util documentation.

      .. note::
         With devices that use software AP-Protect, nRF Util cannot enable hardware Secure AP-Protect if the software Secure AP-Protect is already enabled.
         If you encounter errors with nRF Util, make sure that software AP-Protect and software Secure AP-Protect are disabled.

      **Disabling AP-Protect:**

      If you want to keep the AP-Protect disabled after reset, you must flash firmware that writes ``SwDisable`` to ``APPROTECT.DISABLE`` during boot (``SwDisable`` to ``SECUREAPPROTECT.DISABLE`` during boot for Secure AP-Protect).
      In the |NCS|, :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR` and :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR` (enabled by default) handle the software unlock.

      To disable AP-Protect, issue an ``ERASEALL`` command using the following nRF Util command:

      .. code-block:: console

         nrfutil device recover

      The device is automatically unlocked after erase.
      This command can also disable ``UICR.ERASEPROTECT``.

      **Production programming:**

      For the devices that are in a production environment, it is highly recommended to lock the ``UICR.APPROTECT`` register to prevent unauthorized access to the device. If the access port protection is configured this way, it cannot be disabled without erasing the flash memory.

      Check also the following documentation pages for more information:

      * `Disabling AP-Protect on nRF5340`_
      * `Enabling device protection on nRF5340`_
      * `AP-Protect and ERASEPROTECT troubleshooting for nRF5340`_

.. _app_approtect_nrf52_series:

nRF52 Series
============

.. tabs::

   .. tab:: nRF52840

      .. list-table:: nRF52840 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✔
           - ✔ (depending on HW build code)
           - ✗
           - `AP-Protect for nRF52840`_

      The following Kconfig options configure AP-Protect on the nRF52840 in the |NCS|:

      .. nrf52840_approtect_tab_start

      .. list-table:: AP-Protect configuration options in the |NCS|
         :header-rows: 1
         :align: center
         :widths: auto

         * - Desired AP-Protect state
           - Kconfig option
           - Description
           - Applicability
         * - Enabled
           - :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK`
           - With this Kconfig option set, the MDK locks AP-Protect in ``SystemInit()`` at every boot. UICR is not modified by this Kconfig option.
           - Only HW build codes supporting hardware and software AP-Protect
         * - Disabled
           - :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR`
           - | With this Kconfig option set, AP-Protect follows the UICR register. If UICR is open (``UICR.APPROTECT`` disabled), AP-Protect is disabled. This is the default in the |NCS|.
             | You can start debugging the firmware without additional steps needed.
           - All HW build codes

      **Enabling AP-Protect:**

      To enable hardware AP-Protect, write ``Enabled`` to ``UICR.APPROTECT`` to lock the register and reset the device.
      For example, you can use the following nRF Util command:

      .. code-block:: console

         nrfutil device protection-set All

      This command enables the hardware AP-Protect and resets the device.
      For more information about this command, see `Configuring readback protection`_ in the nRF Util documentation.

      **Disabling AP-Protect:**

      If your device supports hardware and software AP-Protect and you want to keep the AP-Protect disabled after reset, you must flash firmware that writes ``SwDisable`` to ``APPROTECT.DISABLE`` during boot.
      In the |NCS|, :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR` handles the software unlock.

      To disable AP-Protect, issue an ``ERASEALL`` command using the following nRF Util command:

      .. code-block:: console

         nrfutil device recover

      The device is automatically unlocked after erase.

      .. nrf52840_approtect_tab_end

   .. tab:: nRF52833

      .. list-table:: nRF52833 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✔
           - ✔ (depending on HW build code)
           - ✗
           - `AP-Protect for nRF52833`_

      The following Kconfig options configure AP-Protect on the nRF52833 in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf52840_approtect_tab_start
         :end-before: .. nrf52840_approtect_tab_end

   .. tab:: nRF52832

      .. list-table:: nRF52832 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✔
           - ✔ (depending on HW build code)
           - ✗
           - `AP-Protect for nRF52832`_

      The following Kconfig options configure AP-Protect on the nRF52832 in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf52840_approtect_tab_start
         :end-before: .. nrf52840_approtect_tab_end

   .. tab:: nRF52820

      .. list-table:: nRF52820 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✔
           - ✔ (depending on HW build code)
           - ✗
           - `AP-Protect for nRF52820`_

      The following Kconfig options configure AP-Protect on the nRF52820 in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf52840_approtect_tab_start
         :end-before: .. nrf52840_approtect_tab_end

   .. tab:: nRF52811

      .. list-table:: nRF52811 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✔
           - ✔ (depending on HW build code)
           - ✗
           - `AP-Protect for nRF52811`_

      The following Kconfig options configure AP-Protect on the nRF52811 in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf52840_approtect_tab_start
         :end-before: .. nrf52840_approtect_tab_end

   .. tab:: nRF52810

      .. list-table:: nRF52810 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✔
           - ✔ (depending on HW build code)
           - ✗
           - `AP-Protect for nRF52810`_

      The following Kconfig options configure AP-Protect on the nRF52810 in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf52840_approtect_tab_start
         :end-before: .. nrf52840_approtect_tab_end

   .. tab:: nRF52805

      .. list-table:: nRF52805 AP-Protect support
         :header-rows: 1
         :align: center
         :widths: auto

         * - Hardware AP-Protect
           - Hardware + Software AP-Protect
           - Secure AP-Protect
           - HW documentation
         * - ✔
           - ✔ (depending on HW build code)
           - ✗
           - `AP-Protect for nRF52805`_

      The following Kconfig options configure AP-Protect on the nRF52805 in the |NCS|:

      .. include:: ./ap_protect.rst
         :start-after: .. nrf52840_approtect_tab_start
         :end-before: .. nrf52840_approtect_tab_end
