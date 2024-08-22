.. _companion_components:

Using companion components
##########################

.. contents::
   :local:
   :depth: 2

The |NCS| provides several companion components.
Typically, a companion component is independent from the application and is included in your project as a separate firmware image.

Companion components are intended to be used as-is, but integrating them with your application can require minimal modifications of both the component and your application.
Refer to the component documentation for tested and supported integration instructions (see the links in the table).

While you can also significantly modify these components, they are only tested for the default configuration set provided in the |NCS|.

Depending on its function, a companion component image can either run on the same core as the application, as in the case of :ref:`bootloader` and :ref:`ug_tfm`, or run on a separate core, as in the case of :ref:`ipc_radio`.

You can add some of the components, such as :ref:`ipc_radio`, using :ref:`configuration_system_overview_sysbuild`.

The following table lists the available companion components:

.. list-table::
   :header-rows: 1

   * - Component
     - Purpose
     - Supported platforms
     - Location
     - Integration instructions
   * - :ref:`ipc_radio`
     - Companion component that allows to use the radio peripheral from another core in a multicore device.
     - :ref:`nRF54H20 DK <ug_nrf54h20_gs>`, :ref:`nRF5340 DK <ug_nrf5340>`, :ref:`Thingy:53 <ug_thingy53>`
     - :file:`applications/ipc_radio`
     - :ref:`Application's configuration section <ipc_radio_config>`
   * - :ref:`MCUboot <mcuboot:mcuboot_wrapper>`
     - Secure bootloader for 32-bit microcontrollers.
     - :ref:`All supported boards <app_boards_names>` **except** the nRF54H20 DK
     - :file:`ncs/bootloader/mcuboot` (external module), :file:`ncs/nrf/modules/mcuboot` (integration files)
     - :ref:`mcuboot:mcuboot_ncs`
   * - `Trusted Firmware-M (TF-M) <TF-M documentation_>`_
     - Platform security architecture reference implementation aligning with PSA Certified guidelines, enabling chips, Real Time Operating Systems and devices to become PSA Certified.
     - :ref:`nRF91 Series devices <ug_nrf91>`, :ref:`nRF54L15 PDK <ug_nrf54l>`, :ref:`nRF54H20 DK <ug_nrf54h20_gs>`, :ref:`nRF5340 DK <ug_nrf5340>`, :ref:`Thingy:53 <ug_thingy53>`
     - :file:`ncs/modules/tee/tf-m` (external module), :file:`ncs/nrf/modules/trusted-firmware-m` (integration files)
     - :ref:`ug_tfm`
   * - :ref:`bootloader`
     - Bootloader tailored for the :ref:`two-stage bootloader <immutable_bootloader>`.
     - :ref:`Bootloader requirements <bootloader_rot>`
     - :file:`samples/bootloader`
     - :ref:`ug_bootloader_adding_immutable_b0`
   * - :ref:`SUIT flash companion <suit_flash_companion>`
     - Companion image that allows the Secure Domain Firmware to access the external memory during the :ref:`Software Updates for Internet of Things (SUIT) <ug_nrf54h20_suit_dfu>` firmware upgrade.
     - :ref:`Sample requirements <suit_flash_companion_reqs>`
     - :file:`samples/suit/flash_companion`
     - :ref:`Sample's configuration section <suit_flash_companion_config>`
   * - :ref:`SUIT flash recovery image <suit_recovery>`
     - Companion image that allows recovering the device firmware if the original firmware is damaged during the :ref:`Software Updates for Internet of Things (SUIT) <ug_nrf54h20_suit_dfu>` firmware upgrade.
     - :ref:`Sample requirements <suit_recovery_reqs>`
     - :file:`samples/suit/recovery`
     - :ref:`Sample's building and running section <suit_recovery_build_run>`
