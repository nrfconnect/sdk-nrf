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
   * - Hardware
     - Disabled
     - Writing ``Enabled`` to ``UICR.APPROTECT`` and performing a reset.
     - Issuing an ``ERASEALL`` command using CTRL-AP.
       This command erases the flash, UICR, and RAM, including ``UICR.APPROTECT``.
   * - Hardware and software
     - Enabled
     - When AP-Protect is disabled, a reset or a wake enables the access port protection again.
     - Issuing an ``ERASEALL`` command through CTRL-AP.
       This command erases the flash, UICR, and RAM, including ``UICR.APPROTECT``.

       To keep the AP-Protect disabled, ``UICR.APPROTECT`` must be programmed to ``HwDisabled`` and the firmware must write ``SwDisable`` to ``APPROTECT.DISABLE``.

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
     - Also supports Secure AP-Protect (see note below)
   * - nRF9151
     - n/a
     - ✔
     - `AP-Protect for nRF9151`_
     - Also supports Secure AP-Protect (see note below)
   * - nRF9131
     - n/a
     - ✔
     - *Documentation not yet available*
     - Also supports Secure AP-Protect (see note below)
   * - nRF9160
     - ✔
     - n/a
     - `Debugger access protection for nRF9160`_
     - Also supports Secure AP-Protect (see note below)
   * - nRF54H20
     - n/a
     - n/a
     - n/a
     - Uses the :ref:`lifecycle state management <ug_nrf54h20_architecture_lifecycle>` mechanism exclusively
   * - nRF54L15
     - n/a
     - ✔
     - *Documentation not yet available*
     - Also supports Secure AP-Protect (see note below)
   * - nRF5340
     - n/a
     - ✔
     - `AP-Protect for nRF5340`_
     - Also supports Secure AP-Protect (see note below)
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

.. note::
    The SoCs or SiPs that support `ARM TrustZone`_ and different :ref:`app_boards_spe_nspe` (nRF5340, nRF54L15 and nRF91 Series) implement two AP-Protect systems: AP-Protect and Secure AP-Protect.
    While AP-Protect blocks access to all CPU registers and memories, Secure AP-Protect limits access to the CPU to only non-secure accesses.
    This means that the CPU is entirely unavailable while it is running the code in the Secure Processing Environment, and only non-secure registers and address-mapped resources can be accessed.

.. _app_approtect_ncs:

Configuration overview in the |NCS|
***********************************

Based on the available implementation types, you can configure the access port protection mechanism in the |NCS| to one of the following states:

.. list-table:: AP-Protect states
   :header-rows: 1
   :align: center
   :widths: auto

   * - AP-Protect state
     - Related Kconfig option in the |NCS|
     - Description of the AP-Protect state
     - AP-Protect implementation type
   * - Locked
     - :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` (:kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK` for Secure AP-Protect)
     - In this state, CPU uses the MDK system start-up file to enable and lock AP-Protect. UICR is not modified.
     - Hardware and software
   * - Authenticated
     - :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING` (:kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING` for Secure AP-Protect)
     - In this state, AP-Protect is left enabled and it is up to the user-space code to handle unlocking the device if needed.
       The MDK will close the debug AHB-AP, but not lock it, so the AHB-AP can be reopened by the firmware.
       Reopening the AHB-AP should be preceded by a handshake operation over UART, CTRL-AP Mailboxes, or some other communication channel.
     - Hardware and software
   * - Open (default)
     - | :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR` (:kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR` for Secure AP-Protect)
       |
       | This option is set to ``y`` by default in the |NCS|.
     - In this state, AP-Protect follows the UICR register. If the UICR is open, meaning ``UICR.APPROTECT`` has the value ``Disabled``, the AP-Protect will be disabled. (The exact value, placement, the enumeration name, and format varies between nRF Series families.)
     - Hardware; hardware and software

.. _app_approtect_ncs_lock:

Enabling software AP-Protect with :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK`
=============================================================================

Setting the :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` Kconfig option to ``y`` and compiling the firmware enables the software access protection mechanism for SoCs of the nRF53 Series and the SoC revisions of the nRF52 Series that feature the hardware and software type of AP-Protect.

Enabling the Kconfig option writes the debugger register in the ``SystemInit()`` function to lock the access port protection at every boot.
In addition to this, the ``UICR.APPROTECT`` register should be written as instructed in :ref:`app_approtect_uicr_approtect`.

.. note::
    For multi-image builds, this Kconfig option needs to be set for the first image (usually a bootloader).
    Otherwise, the software AP-Protect will not be sufficient as the debugger can be attached to the device after the first image opens the software AP-Protect with the :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR` Kconfig option, which is the default value.

    When using sysbuild, set the ``SB_CONFIG_APPROTECT_LOCK`` sysbuild Kconfig option, which enables the :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` Kconfig option for all images.

.. important::
    On the nRF91x1 Series devices, the register setting related to the :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` Kconfig option does not persist in System ON IDLE mode.
    You must lock the ``UICR.APPROTECT`` register to enable the hardware AP-Protect mechanism as instructed in :ref:`app_approtect_uicr_approtect`.

.. _app_approtect_ncs_user_handling:

Enabling software AP-Protect with :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING`
======================================================================================

Setting the :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING` Kconfig option to ``y`` and compiling the firmware allows you to handle the state of the software AP-Protect at a later stage.
This option in fact does not touch the mechanism and keeps it closed.

You can use this option for example to implement the authenticated debug and lock.
See the SoC or SiP hardware documentation for more information.

.. note::
    For multi-image builds, this Kconfig option has to be set for all images.
    The default value is to open the device if the ``UICR.APPROTECT`` register is not set.
    This allows the debugger to be attached to the device.

    When using sysbuild, set the ``SB_CONFIG_APPROTECT_USER_HANDLING`` sysbuild Kconfig option, which enables the :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING` Kconfig option for all images.

.. _app_approtect_ncs_use_uicr:

Enabling software AP-Protect with :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR`
=================================================================================

Setting the :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR` Kconfig option to ``y`` and compiling the firmware makes the software AP-Protect disabled by default.
This is the default setting in the |NCS|.

You can start debugging the firmware without additional steps needed.

.. _app_approtect_uicr_approtect:

Enabling hardware AP-Protect by locking the ``UICR.APPROTECT`` register
=======================================================================

For the devices that are in a production environment, it is highly recommended to lock the ``UICR.APPROTECT`` register to prevent unauthorized access to the device.
If the access port protection is configured this way, it cannot be disabled without erasing the flash memory.

.. note::
    This is the only mechanism supported by the nRF52 Series and the nRF9160 devices that do not support both hardware and software AP-Protect.

To lock the ``UICR.APPROTECT`` register, complete the following steps:

.. code-block:: console

   nrfjprog --rbp ALL

.. note::
    |nrfjprog_deprecation_note|

This command enables the hardware AP-Protect (and Secure AP-Protect) and resets the device.

.. _app_secure_approtect:

Secure AP-Protect
=================

With :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` comes :ref:`security by separation <app_boards_spe_nspe>`, enabling a Secure Processing Environment (SPE) that is isolated from the Non-Secure Processing Environment (NSPE).
TF-M is available for the nRF53 and nRF91 Series devices.

While AP-Protect blocks access to all CPU registers and memories, Secure AP-Protect limits the CPU access to the non-secure side only.
This allows debugging of the NSPE, while the SPE debugging is blocked.

The following Kconfig options for enabling Secure AP-Protect are available for the nRF91x1 and nRF53 Series devices:

* :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK`
* :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING`
* :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR`

In addition, you can enable hardware Secure AP-Protect by setting the ``UICR.SECUREAPPROTECT`` register as instructed in :ref:`app_secure_approtect_uicr_approtect`.

Enabling software Secure AP-Protect with :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK`
-------------------------------------------------------------------------------------------

Setting the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK` Kconfig option to ``y`` and compiling the firmware enables the secure access protection mechanism for SoCs of the nRF53 Series.

Enabling this Kconfig option writes the secure debugger register in the ``SystemInit()`` function to lock the secure access port protection at every boot.
In addition to this, the ``UICR.SECUREAPPROTECT`` register should be written as instructed in :ref:`app_secure_approtect_uicr_approtect`.

.. note::
    For multi-image builds, this Kconfig option needs to be set for the first image (usually a bootloader).
    Otherwise, the software Secure AP-Protect will not be sufficient as the debugger can be attached to the SPE after the first image opens the software Secure AP-Protect with the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR` Kconfig option, which is the default value.

    When using sysbuild, set the sysbuild Kconfig option ``SB_CONFIG_SECURE_APPROTECT_LOCK``, which enables the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK` Kconfig option for all images.

.. important::
    On the nRF91x1 Series devices, the register setting related to the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK` Kconfig option does not persist in System ON IDLE mode.
    You must lock the ``UICR.SECUREAPPROTECT`` register to enable the hardware Secure AP-Protect mechanism as instructed in :ref:`app_secure_approtect_uicr_approtect`.

Enabling software Secure AP-Protect with :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING`
----------------------------------------------------------------------------------------------------

Setting the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING` Kconfig option to ``y`` and compiling the firmware allows you to handle the state of the software Secure AP-Protect at a later stage.
This option does not touch the mechanism and keeps it closed.

You can for example use this option to implement an authenticated debug and lock of the SPE.
See the SoC or SiP hardware documentation for more information.

.. note::
    For multi-image builds, this Kconfig option needs to be set for all images.
    The default value is to open the device if the ``UICR.SECUREAPPROTECT`` is not set.
    This allows the debugger to be attached to the device.

    When using sysbuild, set the ``SB_CONFIG_SECURE_APPROTECT_USER_HANDLING`` sysbuild Kconfig option, which enables the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING` Kconfig option for all images.

Enabling software Secure AP-Protect with :kconfig:option:`CONFIG_SECURE_NRF_APPROTECT_USE_UICR`
-----------------------------------------------------------------------------------------------

Setting the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR` Kconfig option to ``y`` and compiling the firmware disables the software Secure AP-Protect mechanism by default.
This is the default setting in the |NCS|.

You can start debugging the SPE without additional steps needed.

.. _app_secure_approtect_uicr_approtect:

Enabling hardware Secure AP-Protect by locking the ``UICR.SECUREAPPROTECT`` register
------------------------------------------------------------------------------------

To enable only the hardware Secure AP-Protect mechanism, run the following command:

.. note::
    This is the only mechanism supported for the nRF9160 devices that do not have software support for Secure AP-Protect.

.. code-block:: console

   nrfjprog --rbp SECURE

.. note::
    |nrfjprog_deprecation_note|

This command enables hardware Secure AP-Protect and resets the device.
