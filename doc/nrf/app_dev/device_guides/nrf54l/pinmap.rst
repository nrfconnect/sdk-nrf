.. _ug_nrf54l_pinmap:

nRF54L pin mapping
##################

.. contents::
   :local:
   :depth: 2

The nRF54L Series SoCs manage power for peripherals and pins automatically when both are in the same domain.
The SoCs also support mapping pins across different domains, but this requires additional firmware management and periodic use of the Constant Latency sub-power mode.
As a result, this increases both latency and power consumption.

Mapping pins across power-domains
*********************************

The following sections explain how to map and manage pins within a single domain and across different domains.

Understanding cross power-domain pin mapping
============================================

For information on possible pin assignments, refer to the *Pin assignments* chapter in the respective device’s datasheet.
The *Dedicated pins* and *Cross power-domain use* subsections document which pins can be connected across different domains.
See :ref:`ug_nrf54l` for a complete list of references.

Managing cross power-domain pin mapping
=======================================

To use a peripheral with pins mapped across different domains, you must enable the Constant Latency sub-power mode.
You can do this by setting the :kconfig:option:`CONFIG_NRF_SYS_EVENT` Kconfig option, and calling the :c:func:`nrf_sys_event_request_global_constlat` function in your application.

See the following example:

.. code-block:: c

   #include <nrf_sys_event.h>

   int main(void)
   {
           /* Request constlat. The API is reference counted. */
           nrf_sys_event_request_global_constlat();

           /* Use peripherals which have pins mapped across power-domains */

           /* Release constlat */
           nrf_sys_event_release_global_constlat();

           return 0;
   }
