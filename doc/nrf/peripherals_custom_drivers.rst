.. _peripherals_custom_drivers:

Peripherals custom drivers
##########################

Nordic Semiconductor SoCs contain a variety of hardware peripherals that can be used for many purposes in the context of |NCS| applications.
Many of them can be utilized using standardized `Zephyr's device driver API`_, however this generalized approach may limit some of peripheral capabilities.
To overcome this issue peripheral custom driver can be implemented.
Peripheral custom driver utilizes one or many `nrfx`_ drivers and, potentially, `nrfx_gppi helper layer`_, then wrapping them into `Zephyr's device driver API`_, allowing to fully leverage the potential of Nordic SoCs hardware peripherals.

The following pages describe peripheral custom drivers in the |NCS| that can be used as a reference.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:
   :glob:
