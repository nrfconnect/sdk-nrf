.. _app_approtect:

Enabling access port protection mechanism
#########################################

.. contents::
   :local:
   :depth: 2

.. app_approtect_info_start

Several Nordic Semiconductor SoCs supported in the |NCS| offer an implementation of the access port protection mechanism (AP-Protect).
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
     - Writing ``UICR.APPROTECT`` to ``Enabled`` and performing a reset.
     - Issuing an ``ERASEALL`` command via CTRL-AP.
       This command erases the flash, UICR, and RAM, including ``UICR.APPROTECT``.
   * - Hardware and software
     - Enabled
     - When AP-Protect is disabled, a reset or a wake enables the access port protection again.
     - Issuing an ``ERASEALL`` command via CTRL-AP.
       This command erases the flash, UICR, and RAM, including ``UICR.APPROTECT``.

       To keep the AP-Protect disabled, ``UICR.APPROTECT`` must be programmed to ``HwDisabled`` and the firmware must write ``APPROTECT.DISABLE`` to ``SwDisable``.

The following table lists related SoCs with information about the AP-Protect mechanism they support.
For some SoCs, the AP-Protect implementation is different depending on the build code of the device.
See the related hardware documentation for more information about which implementation is supported for which build code and about the differences between the supported implementations.

.. list-table:: SoC AP-Protect matrix
   :header-rows: 1
   :align: center
   :widths: auto

   * - SoC
     - Hardware AP-Protect support
     - Hardware and software AP-Protect support
     - Related hardware documentation
     - Additional information
   * - nRF9160
     - ✔
     -
     - `Debugger access protection for nRF9160`_
     - Also supports Secure AP-Protect (see note below)
   * - nRF5340
     -
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
    The SoCs that support `ARM TrustZone`_ and different :ref:`app_boards_spe_nspe` (nRF5340 and nRF9160) implement two AP-Protect systems: AP-Protect and Secure AP-Protect.
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
     - Description
     - AP-Protect implementation type
   * - Locked
     - :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` (:kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK` for Secure AP-Protect)
     - In this state, CPU writes to enable and lock AP-Protect. UICR is not modified.
     - Hardware and software
   * - Authenticated
     - :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING` (:kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING` for Secure AP-Protect)
     - In this state, AP-Protect is left enabled and it is up to the user handler to unlock the device if needed.
     - Hardware and software
   * - Open
     - :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR` (:kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR` for Secure AP-Protect)
     - In this state, AP-Protect follows the UICR register. If the UICR is open, the AP-Protect will be disabled.
     - Hardware; hardware and software

.. _app_approtect_ncs_lock:

Enabling AP-Protect with :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK`
====================================================================

Setting the :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` Kconfig option to ``y`` and compiling the firmware is enough to enable the access port protection mechanism for SoCs of the nRF53 Series and those SoCs of the nRF52 Series that feature the hardware and software type of AP-Protect.
The access port protection configured in this way cannot be disabled without erasing the flash.

.. _app_approtect_ncs_user_handling:

Enabling AP-Protect with :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING`
=============================================================================

Setting the :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING` Kconfig option to ``y`` and compiling the firmware allows you to handle the state of AP-Protect at a later stage.
This option in fact does not touch the mechanism and keeps it closed.

You can use this option for example to implement the authenticated debug and lock.
See the SoC hardware documentation for more information.

.. _app_approtect_ncs_use_uicr:

Enabling AP-Protect with :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR`
========================================================================

Setting the :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR` Kconfig option to ``y`` and compiling the firmware makes the AP-Protect disabled by default.

You can start debugging the firmware without additional steps needed.

Once you are done debugging, run the following command to enable the access port protection:

.. code-block:: console

   nrfjprog --rbp ALL

This command enables the AP-Protect and resets the device.

To enable only the Secure AP-Protect, run the following command:

.. code-block:: console

   nrfjprog --rbp SECURE
