.. _network_core_monitor:

Network core monitor
####################

.. contents::
   :local:
   :depth: 2

This library monitors the network core status of the nRF5340 processor.

Overview
********

It detects suspensions and resets in the network processor.
When a reset occurs, you can read the cause of the reset.

Implementation
==============

The library uses two general-purpose registers of the IPC peripheral in the application core and writes the state of the network processor to them.

.. figure:: images/ncm_register.svg
   :alt: Application of IPC peripheral registers

   IPC peripheral registers

The ``GPMEM[0]`` register is divided into two 16-bit parts.

The ``COUNTER`` value is incremented by the network core every :kconfig:option:`CONFIG_NCM_APP_FEEDING_INTERVAL_MSEC`.
The application core periodically calls the :c:func:`ncm_net_status_check` function to check the status of the network core.
If the network core is suspended, the counter value is not updated.
The :c:func:`ncm_net_status_check` function returns error code ``-EBUSY``.

To correctly detect the network core status, the :c:func:`ncm_net_status_check` function should be called less frequently than the value set in the :kconfig:option:`CONFIG_NCM_APP_FEEDING_INTERVAL_MSEC` Kconfig option.

The ``FLAGS`` field has the ``Reset`` flag as the bit 0.

The reset bit is set at the boot of the network core.
It informs the application core that a reset of the network core has occurred.

During the startup of the network core, the reset bit is set and the cause of the reset is written to the IPC register ``GPMEM[1]``.
This value is rewritten from the network core's ``RESET.RESETREAS`` register.
For a detailed description of the bits in this register, see the `RESETREAS`_ section for nRF5340.

The :c:func:`ncm_net_status_check` function returns the following error codes:

* ``-EBUSY`` if the network core is suspended
* ``-EFAULT`` if a network core reset has occurred

The function takes a pointer to a variable of type uint32_t as a parameter.
When a network core reset is detected, the cause of the reset is stored in this pointer.

Configuration
*************

To enable this library, set the :kconfig:option:`CONFIG_NET_CORE_MONITOR` Kconfig option to ``y`` on both network and application cores.

The :kconfig:option:`CONFIG_NCM_APP_FEEDING_INTERVAL_MSEC` Kconfig option specifies how often the counter is updated by the network core.
The default value is 500 milliseconds.

The :kconfig:option:`CONFIG_NCM_RESET_INIT_PRIORITY` Kconfig option sets priority for the initialization function.
The function reads the cause of the processor reset and passes this information to the application core.
It is executed at the network core boot.

Usage
*****

To enable the Network core monitor library, set the :kconfig:option:`CONFIG_NET_CORE_MONITOR` Kconfig option.

On the application core, periodically call the :c:func:`ncm_net_status_check` function to monitor the state of the network core.
See the following usage example.

.. code-block::

   #include "net_core_monitor.h"
   ...
   static void print_reset(uint32_t reset_reas)
   {
      if (reset_reas & NRF_RESET_RESETREAS_RESETPIN_MASK) {
         printk("Reset by pin-reset\n");
      } else if (reset_reas & NRF_RESET_RESETREAS_DOG0_MASK) {
         printk("Reset by application watchdog timer 0 \n");
      } else if (reset_reas & NRF_RESET_RESETREAS_SREQ_MASK) {
         printk("Reset by soft-reset\n");
      } else if (reset_reas) {
         printk("Reset by a different source (0x%08X)\n", reset_reas);
      }
   }

   int main(void)
   {
      uint32_t reset_reas;
      ...
      for (;;) {
         ret = ncm_net_status_check(&reset_reas);
         if (ret == -EBUSY) {
            /* do something*/
         } else if (ret == -EFAULT) {
            print_reset(reset_reas);
         }
         k_sleep(K_MSEC(1000));
      }
   }

Dependencies
************

The module uses two general-purpose registers, ``GPMEM[0]`` and ``GPMEM[1]``, of the application core's IPC peripheral.

API documentation
*****************

| Header file: :file:`include/net_core_monitor.h`
| Source files: :file:`subsys/net_core_monitor/`

.. doxygengroup:: net_core_monitor
   :project: nrf
   :members:
