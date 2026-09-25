.. _lib_nrf71_idle_power:

nRF71 idle power
#################

.. contents::
   :local:
   :depth: 2

The nRF71 idle power library provides reusable helpers that lower the System ON idle
current of an application running on the nRF71 Series application core.
The library applies a Kconfig-selected RAM retention level at boot from a
:c:func:`SYS_INIT` hook, so an application does not need any code changes to benefit
from it, and pulls in the device power management Kconfig options that let peripherals
such as the console UART suspend themselves while the system is idle.

RAM retention
*************

The :kconfig:option:`CONFIG_NRF71_IDLE_POWER_RAM_RETAIN` choice selects how much
application RAM stays powered during System ON idle, using the :ref:`lib_ram_pwrdn`
library:

.. list-table::
   :header-rows: 1

   * - Kconfig option
     - Behavior
   * - :kconfig:option:`CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_FULL` (default)
     - No RAM power-down call is made. All application RAM stays retained.
   * - :kconfig:option:`CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_UNUSED_ONLY`
     - Calls :c:func:`power_down_unused_ram` at boot.
       The unused range is computed from the image layout, so live image, stack, and
       buffer memory is never touched.
       This is the recommended tier for a Wi-Fi build.
   * - :kconfig:option:`CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_64K`
     - Calls :c:func:`power_down_ram` at boot to power down everything past the first
       64 KiB of application RAM.
       Only safe if the whole image fits below that boundary.
   * - :kconfig:option:`CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_128K`
     - Same as above, with a 128 KiB boundary.
   * - :kconfig:option:`CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_256K`
     - Same as above, with a 256 KiB boundary.
   * - :kconfig:option:`CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_512K`
     - Same as above, with a 512 KiB boundary.

Powering down RAM that the running image still uses corrupts live data, so the
fixed-size tiers only make sense for a trivial image that fits below the selected
boundary. For a Wi-Fi build, use
:kconfig:option:`CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_UNUSED_ONLY` instead.

Console suspend and resume
***************************

Enabling :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` alone does not suspend a
polling-mode UART console backend, because such a backend never calls
:c:func:`pm_device_runtime_put` on its own: the UART, and whatever clock it
requests, stays fully active for as long as the application is running,
including while it is otherwise idle.
Call :c:func:`nrf71_idle_power_suspend_console` right before an idle
:c:func:`k_sleep` or :c:func:`k_sem_take` call to suspend the device backing
the ``zephyr,console`` chosen node, and :c:func:`nrf71_idle_power_resume_console`
again before the next print. Both calls are no-ops if there is no
``zephyr,console`` chosen node, or if the console device is not ready, so a
sample can call them unconditionally.

Diagnostics
***********

Enable :kconfig:option:`CONFIG_NRF71_IDLE_DIAGNOSTICS` to get
:c:func:`nrf71_idle_power_print_snapshot`, which dumps the POWER, MEMCONF, GRTC, LFXO,
and Wi-Fi core LRC0 registers so they can be correlated with a power analyzer trace.
When the option is disabled, the function is still available as a no-op inline, so an
application can call it unconditionally.

The option also configures a GPIO idle-phase marker from the ``idle-phase``
devicetree alias, when the application provides one, and, when
:kconfig:option:`CONFIG_SHELL` is enabled, registers an ``idle_snapshot`` shell command
for on-demand snapshots.

API documentation
******************

| Header file: :file:`include/nrf71_idle_power.h`
| Source files: :file:`lib/nrf71_idle_power/`

.. doxygengroup:: nrf71_idle_power
