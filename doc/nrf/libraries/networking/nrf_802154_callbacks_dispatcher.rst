.. _nrf_802154_callbacks_dispatcher:

nRF 802.15.4 callbacks dispatcher
#################################

.. contents::
   :local:
   :depth: 2

The nRF 802.15.4 callbacks dispatcher routes driver events from the :ref:`nrf_802154` to the protocol stack that currently owns the radio.
Use it when a single firmware image registers more than one nRF IEEE 802.15.4 radio driver client (for example, OpenThread and Zigbee in a coexistence build) and each client provides its own callback implementation.

Overview
********

The nRF IEEE 802.15.4 radio driver invokes a fixed set of callouts (receive, transmit done, energy detect, and related notifications).
Only one implementation of each callout can be linked into the image.

The dispatcher performs the following operations:

* Registers clients at build time through an iterable linker section.
* Exposes the driver callouts and forwards each event to the active client's :c:struct:`nrf_802154_callbacks` table.
* Switches the active client at runtime using the :c:func:`nrf_802154_callbacks_dispatcher_switch` function.

When switching clients, the dispatcher deinitializes the previous client, reinitializes the radio driver to a known default state, initializes the new client, and optionally applies radio addresses returned by the client's ``get_config`` callback.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_NRF_802154_CALLBACKS_DISPATCHER` Kconfig option to ``y``.

The option also selects :kconfig:option:`CONFIG_NRF_802154_DRV_REINIT_ENABLED`, which is required for runtime client switches.

You can adjust the SYS_INIT priority using the :kconfig:option:`CONFIG_NRF_802154_CALLBACKS_DISPATCHER_INIT_PRIORITY` Kconfig option (default ``80``).

Usage
*****

Register each client in file scope using the :c:macro:`NRF_802154_CALLBACKS_DISPATCHER_REGISTER` macro and a :c:struct:`nrf_802154_callbacks` structure.
The macro argument becomes the client name passed to the :c:func:`nrf_802154_callbacks_dispatcher_switch` function.

.. code-block:: c

   static struct nrf_802154_callbacks my_stack_cbs = {
	   .init = my_stack_init,
	   .deinit = my_stack_deinit,
	   .received_timestamp_raw = my_stack_received,
	   /* Other callbacks as needed; unused entries may be NULL. */
   };

   NRF_802154_CALLBACKS_DISPATCHER_REGISTER(my_stack, my_stack_cbs);

Activate a client before using the radio as follows:

.. code-block:: c

   err = nrf_802154_callbacks_dispatcher_switch("my_stack");
   if (err) {
	   /* Handle error */
   }

Pass ``NULL`` or an empty string to deactivate the current client without selecting a new one.
The function returns ``-EINVAL`` if the name does not match a registered client.

All callbacks in the :c:struct:`nrf_802154_callbacks` structure are optional.
If the active client leaves a function pointer as ``NULL``, the corresponding driver event is ignored.

Dependencies
************

The library depends on the nRF IEEE 802.15.4 radio driver headers and must be linked together with a single driver instance in the application.
Coexistence logic (when to call the :c:func:`nrf_802154_callbacks_dispatcher_switch` function) is provided by the application or a higher-level module.

API documentation
*****************

| Header file: :file:`include/net/nrf_802154_callbacks_dispatcher.h`
| Source file: :file:`lib/nrf_802154_callbacks_dispatcher/nrf_802154_callbacks_dispatcher.c`

.. doxygengroup:: nrf_802154_callbacks_dispatcher
