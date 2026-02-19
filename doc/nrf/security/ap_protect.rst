.. _app_approtect:

Enabling access port protection mechanism
#########################################

.. contents::
   :local:
   :depth: 2

.. app_approtect_info_start

Several Nordic Semiconductor SoCs or SiPs supported in the |NCS| offer an implementation of the access port protection mechanism (AP-Protect).
When enabled, this mechanism blocks the debugger from read and write access to all CPU registers and memory-mapped addresses.
Accessing these registers and addresses again requires disabling the mechanism and erasing the flash.

.. app_approtect_info_end

.. _app_approtect_implementation_overview:

Implementation overview
***********************

The following table provides a general overview of the available access port protection mechanisms in Nordic Semiconductor devices.
For detailed information, refer to the hardware documentation.

.. list-table:: AP-Protect implementations
   :header-rows: 1
   :align: center
   :widths: auto

   * - AP-Protect implementation type
     - Default factory state
     - How to enable
     - How to disable
   * - Hardware and IronSide SE
     - Disabled
     - Writing ``Enabled`` to ``UICR.APPROTECT`` and performing a reset.
     - Issuing an ``ERASEALL`` command using CTRL-AP.
       This command erases the flash, UICR, and RAM, including ``UICR.APPROTECT``.
   * - Hardware and software
     - Enabled
     - When AP-Protect is disabled, a reset or a wake enables the access port protection again.
     - Issuing an ``ERASEALL`` command through CTRL-AP.
       This command erases the flash, UICR, and RAM, including ``UICR.APPROTECT``.

       To keep the AP-Protect disabled, ``UICR.APPROTECT`` must be programmed to ``HwDisabled``.

       Additionally:

       - For nRF52, nRF53, and nRF91 devices, firmware must write ``SwDisable`` to ``APPROTECT.DISABLE``.

       - For nRF54L devices, firmware must open the `debugger signals in Tamper Controller <nRF54L15 Debugger signals_>`_.

The following table lists related SoCs or SiPs with information about the AP-Protect mechanism they support.
For some SoCs or SiPs, the AP-Protect implementation is different depending on the build code of the device.
See the related hardware documentation for more information about which implementation is supported for which build code and about the differences between the supported implementations.

.. list-table:: SoC or SiP AP-Protect matrix
   :header-rows: 1
   :align: center
   :widths: auto

   * - SoC or SiP
     - Hardware AP-Protect support
     - Hardware and software AP-Protect support
     - Related hardware documentation
     - Additional information
   * - nRF9161
     - n/a
     - ✔
     - `AP-Protect for nRF9161`_
     - Also :ref:`supports Secure AP-Protect <secure_approtect_support>`
   * - nRF9151
     - n/a
     - ✔
     - `AP-Protect for nRF9151`_
     - Also :ref:`supports Secure AP-Protect <secure_approtect_support>`
   * - nRF9131
     - n/a
     - ✔
     - *Documentation not yet available*
     - Also :ref:`supports Secure AP-Protect <secure_approtect_support>`
   * - nRF9160
     - ✔
     - n/a
     - `Debugger access protection for nRF9160`_
     - Also :ref:`supports Secure AP-Protect <secure_approtect_support>`
   * - nRF54H20
     - ✔
     - n/a
     - n/a
     - See :ref:`UICR.APPROTECT <ug_nrf54h20_ironside_se_uicr_approtect>`.
   * - nRF54LV10A
     - n/a
     - ✔
     - `AP-Protect for nRF54LV10A`_
     - Also :ref:`supports Secure AP-Protect <secure_approtect_support>`
   * - nRF54LM20
     - n/a
     - ✔
     - `AP-Protect for nRF54LM20A`_
     - Also :ref:`supports Secure AP-Protect <secure_approtect_support>`
   * - nRF54L15
     - n/a
     - ✔
     - `AP-Protect for nRF54L15`_
     - Also :ref:`supports Secure AP-Protect <secure_approtect_support>`
   * - nRF5340
     - n/a
     - ✔
     - `AP-Protect for nRF5340`_
     - Also :ref:`supports Secure AP-Protect <secure_approtect_support>`
   * - nRF52840
     - ✔
     - ✔
     - `AP-Protect for nRF52840`_
     -
   * - nRF52833
     - ✔
     - ✔
     - `AP-Protect for nRF52833`_
     -
   * - nRF52832
     - ✔
     - ✔
     - `AP-Protect for nRF52832`_
     -
   * - nRF52820
     - ✔
     - ✔
     - `AP-Protect for nRF52820`_
     -
   * - nRF52811
     - ✔
     - ✔
     - `AP-Protect for nRF52811`_
     -
   * - nRF52810
     - ✔
     - ✔
     - `AP-Protect for nRF52810`_
     -
   * - nRF52805
     - ✔
     - ✔
     - `AP-Protect for nRF52805`_
     -

.. _secure_approtect_support:

Secure AP-Protect support
=========================

The SoCs or SiPs that support `ARM TrustZone`_ and different :ref:`app_boards_spe_nspe` (nRF5340, nRF54L15, and nRF91 Series devices) implement two AP-Protect systems: AP-Protect and Secure AP-Protect.

- AP-Protect blocks access to all CPU registers and memories
- Secure AP-Protect limits access to the CPU to only non-secure accesses.
  This means that the CPU is entirely unavailable while it is running the code in the Secure Processing Environment, and only non-secure registers and address-mapped resources can be accessed.

For information about how to configure Secure AP-Protect in the |NCS|, see :ref:`app_secure_approtect`.

.. _app_approtect_ncs:

Configuration overview in the |NCS|
***********************************

Based on the available implementation types, you can configure the access port protection mechanism in the |NCS| to one of the following states:

.. list-table:: AP-Protect states
   :header-rows: 1
   :align: center
   :widths: auto

   * - AP-Protect state
     - Series or devices
     - Related Kconfig option in the |NCS|
     - Description of the AP-Protect state
     - AP-Protect implementation type
   * - Locked
     - All Series and devices except nRF54H20
     - :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` (:kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK` for Secure AP-Protect)
     - In this state, CPU uses the MDK system start-up file to enable and lock AP-Protect. UICR is not modified.
     - Hardware and software
   * - Authenticated
     - nRF53, nRF54L and nRF91 Series
     - :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING` (:kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING` for Secure AP-Protect)
     - In this state, AP-Protect is left enabled and it is up to the user-space code to handle unlocking the device if needed.
       The MDK will close the debug AHB-AP, but not lock it, so the AHB-AP can be reopened by the firmware.
       Reopening the AHB-AP should be preceded by a handshake operation over UART, CTRL-AP Mailboxes, or some other communication channel.
     - Hardware and software
   * - Open
     - Default for nRF52, nRF53, and nRF91 Series
     - | :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR` (:kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR` for Secure AP-Protect)
       |
       | This option is set to ``y`` by default in the |NCS|.
     - In this state, AP-Protect follows the UICR register. If the UICR is open, meaning ``UICR.APPROTECT`` has the value ``Disabled``, the AP-Protect will be disabled. (The exact value, placement, the enumeration name, and format varies between nRF Series families.)
     - Hardware; hardware and software
   * - Open
     - Default for the nRF54L Series
     - | :kconfig:option:`CONFIG_NRF_APPROTECT_DISABLE` (:kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_DISABLE` for Secure AP-Protect)
       |
       | This option is set to ``y`` by default in the |NCS|.
     - In this state, AP-Protect is disabled.
     - Hardware and software

.. _app_approtect_ncs_lock:

Enabling software AP-Protect with :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK`
=============================================================================

This option is valid for the nRF53 Series, the nRF54L Series, and the SoC revisions of the nRF52 Series that feature the hardware and software type of AP-Protect (see hardware documentation for more information).

.. important::
    On the nRF91x1 Series devices, the register setting related to the :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` Kconfig option does not persist in System ON IDLE mode.
    You must lock the ``UICR.APPROTECT`` register to enable the hardware AP-Protect mechanism as instructed in :ref:`app_approtect_uicr_approtect`.

Setting the :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` Kconfig option to ``y`` and compiling the firmware enables the software access protection mechanism for supported SoCs.

Enabling the Kconfig option writes the debugger register in the ``SystemInit()`` function to lock the access port protection at every boot.
For hardware protection, the ``UICR.APPROTECT`` register should be written as instructed in :ref:`app_approtect_uicr_approtect`.

.. note::
    For multi-image builds, :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` needs to be set for the first image (usually a bootloader).
    Otherwise, the software AP-Protect will not be sufficient as the debugger can be attached to the device after the first image opens the software AP-Protect, which is the default operation.

    You can set this option manually or use sysbuild's :kconfig:option:`SB_CONFIG_APPROTECT_LOCK` Kconfig option to set it for all images at once.

.. _app_approtect_ncs_user_handling:

Enabling software AP-Protect with :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING`
======================================================================================

This option is valid for the nRF53 Series, the nRF54L Series and the nRF91 Series devices.

Setting the :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING` Kconfig option to ``y`` and compiling the firmware allows you to handle the state of the software AP-Protect at a later stage.
This option in fact does not touch the mechanism and keeps it closed.

You can use this option for example to implement the authenticated debug and lock.
See the SoC or SiP hardware documentation for more information.

.. note::
    For multi-image builds, :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING` needs to be set for all images.
    The default value is to open the device.
    This allows the debugger to be attached to the device.

    You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_APPROTECT_USER_HANDLING` Kconfig option to set it for all images at once.

.. _app_approtect_ncs_use_uicr:

Disabling software AP-Protect with :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR`
==================================================================================

This option is valid for the nRF52 Series, the nRF53 Series, and the nRF91 Series devices.

Setting the :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR` Kconfig option to ``y`` and compiling the firmware makes the software AP-Protect disabled by default.
This is the default setting in the |NCS|.

You can start debugging the firmware without additional steps needed.

Disabling software AP-Protect with :kconfig:option:`CONFIG_NRF_APPROTECT_DISABLE`
=================================================================================

This option is valid for the nRF54L Series devices.

Setting the :kconfig:option:`CONFIG_NRF_APPROTECT_DISABLE` Kconfig option to ``y`` and compiling the firmware disables the software AP-Protect.
This is the default setting in the |NCS|.

You can start debugging the firmware without additional steps needed.

.. _app_approtect_uicr_approtect:

Enabling hardware AP-Protect by locking the ``UICR.APPROTECT`` register
=======================================================================

For the devices that are in a production environment, it is highly recommended to lock the ``UICR.APPROTECT`` register to prevent unauthorized access to the device.
If the access port protection is configured this way, it cannot be disabled without erasing the flash memory.

.. note::
    This is the only mechanism supported by the nRF52 Series and the nRF9160 devices that do not support both hardware and software AP-Protect.

To lock the ``UICR.APPROTECT`` register, use the following set of commands:

.. tabs::

   .. tab:: SoCs or SiPs other than nRF5340

      .. code-block:: console

         nrfutil device protection-set All

   .. tab:: nRF5340

      .. code-block:: console

         nrfutil device protection-set All --core Network
         nrfutil device protection-set All

This set of commands enables the hardware AP-Protect (and Secure AP-Protect) and resets the device.

.. note::
    With devices that use software AP-Protect, nRF Util cannot enable hardware AP-Protect if the software AP-Protect is already enabled.
    If you encounter errors with nRF Util, make sure that software AP-Protect is disabled.

.. _app_secure_approtect:

Configuring Secure AP-Protect
=============================

With :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` comes :ref:`security by separation <app_boards_spe_nspe>`, enabling a Secure Processing Environment (SPE) that is isolated from the Non-Secure Processing Environment (NSPE).
TF-M is available for the nRF53, nRF54L and nRF91 Series devices.

While AP-Protect blocks access to all CPU registers and memories, Secure AP-Protect limits the CPU access to the non-secure side only.
This allows debugging of the NSPE, while the SPE debugging is blocked.

The following Kconfig options are available for enabling Secure AP-Protect on the listed devices:

.. list-table:: Secure AP-Protect Kconfig options
  :header-rows: 1
  :align: center
  :widths: auto

  * - Option
    - Series or devices
  * - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK` Kconfig option
    - nRF53, nRF54L
  * - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING` Kconfig option
    - nRF53, nRF54L, nRF91x1 devices
  * - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR` Kconfig option
    - nRF53, nRF91x1 devices
  * - :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_DISABLE` Kconfig option
    - nRF54L
  * - Locking the ``UICR.SECUREAPPROTECT`` register with nRF Util
    - All devices

In addition, you can enable hardware Secure AP-Protect by setting the ``UICR.SECUREAPPROTECT`` register as instructed in :ref:`app_secure_approtect_uicr_approtect`.

Enabling software Secure AP-Protect with :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK`
-------------------------------------------------------------------------------------------

This option is valid for the nRF53 and the nRF54L Series devices.

.. important::
    On nRF91x1 devices, the register setting related to the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK` Kconfig option does not persist in System ON IDLE mode.
    You must lock the ``UICR.SECUREAPPROTECT`` register to enable the hardware Secure AP-Protect mechanism as instructed in :ref:`app_secure_approtect_uicr_approtect`.

Setting the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK` Kconfig option to ``y`` and compiling the firmware enables the secure access protection mechanism.

Enabling this Kconfig option writes the secure debugger register in the ``SystemInit()`` function to lock the secure access port protection at every boot.
For hardware protection, the ``UICR.SECUREAPPROTECT`` register should be written as instructed in :ref:`app_secure_approtect_uicr_approtect`.

.. note::
    For multi-image builds, :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK` needs to be set for the first image (usually a bootloader).
    Otherwise, the software Secure AP-Protect will not be sufficient as the debugger can be attached to the SPE after the first image opens the software Secure AP-Protect, which is the default operation.

    You can set this option manually or use sysbuild's :kconfig:option:`SB_CONFIG_SECURE_APPROTECT_LOCK` Kconfig option to enable it for all images.

Enabling software Secure AP-Protect with :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING`
----------------------------------------------------------------------------------------------------

This option is valid for the nRF53 and the nRF54L Series devices, and nRF91x1 devices.

Setting the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING` Kconfig option to ``y`` and compiling the firmware allows you to handle the state of the software Secure AP-Protect at a later stage.
This option does not touch the mechanism and keeps it closed.

You can for example use this option to implement an authenticated debug and lock of the SPE.
See the SoC or SiP hardware documentation for more information.

.. note::
    With multi-image builds, :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING` needs to be set for all images.
    The default value is to open the device.
    This allows the debugger to be attached to the device.

    You can set this option manually for each image or use sysbuild's :kconfig:option:`SB_CONFIG_SECURE_APPROTECT_USER_HANDLING` Kconfig option to set it for all images at once.

Disabling software Secure AP-Protect with :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR`
------------------------------------------------------------------------------------------------

This option is valid for the nRF53 Series and nRF91x1 devices.

Setting the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR` Kconfig option to ``y`` and compiling the firmware disables the software Secure AP-Protect mechanism by default.
This is the default setting in the |NCS|.

You can start debugging the SPE without additional steps needed.

Disabling software Secure AP-Protect with :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_DISABLE`
-----------------------------------------------------------------------------------------------

This option is valid for the nRF54L Series devices.

Setting the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_DISABLE` Kconfig option to ``y`` and compiling the firmware disables the software Secure AP-Protect.
This is the default setting in the |NCS|.

You can start debugging the SPE without additional steps needed.

.. _app_secure_approtect_uicr_approtect:

Enabling hardware Secure AP-Protect by locking the ``UICR.SECUREAPPROTECT`` register
------------------------------------------------------------------------------------

To enable only the hardware Secure AP-Protect mechanism, run the following command:

.. note::
    This option is supported by all devices and it is the most secure way to enable Secure AP-Protect.
    Moreover, this is the only mechanism supported for the nRF9160 devices that do not have software support for Secure AP-Protect.

.. code-block:: console

   nrfutil device protection-set SecureRegions

This command enables hardware Secure AP-Protect and resets the device.

.. note::
    With devices that use software AP-Protect, nRF Util cannot enable hardware Secure AP-Protect if the software Secure AP-Protect is already enabled.
    If you encounter errors with nRF Util, make sure that software AP-Protect and software Secure AP-Protect are disabled.
