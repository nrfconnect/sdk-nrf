Blutooth stack architecture
###########################

The Bluetooth protocol has a layered architecture, and the |NCS| Bluetooth implementation combines components from the Zephyr Project and components developed by Nordic Semiconductor.
This section briefly describes the main components and provides links to pages with additional information.

The following diagram shows the main components of a Bluetooth application, with a focus on the protocol stack.

.. figure:: images/bluetooth_arch.svg
   :alt: Bluetooth stack components in the |NCS|

   Bluetooth stack components in the |NCS|

Physical layer (PHY)
********************

The Bluetooth Low Energy radio PHY supports 4 different modes, each with different bandwidth and range:

* S8 Coded PHY, 125kbps, longest range, optional support
* S2 Coded PHY, 500kbps, optional support
* 1 Mbps, Mandatory support
* 2 Mbps, shortest range, optional support

Key features required for Bluetooth Direction Finding support are also part of the PHY layer.

Feature support for PHY features depend on specific SoCs.
Please refer to the Bluetooth feature support table on the :ref:`software_maturity` page.

Controller
**********

The :ref:`nrfxlib:softdevice_controller` is the supported Bluetooth controller in the |NCS|.
Application examples in the SDK use the SoftDevice Controller as the default controller, and it is the controller used for release testing of the SDK.

The Bluetooth support includes:

* Implementation of new Bluetooth LE controller features.
* Valid qualification (QDID) for each SDK release.

The Zephyr Project has a community support :ref:`ug_ble_controller`.
This Bluetooth controller will be the default when using Zephyr RTOS stand-alone.
It is possible to configure projects in the |NCS| to use the Zephyr Controller, but Nordic Semiconductor does not support this configuration for production.

Zephyr Host
***********

The |NCS| uses the Bluetooth Host implementation in the Zephyr project for host feature support.
The Zephyr Host implementation is tested with the rest of the SDK for releases, and a valid QDID is provided for each SDK release.

For more information, see the :ref:`bluetooth_le_host` page in the Zephyr documentation set.
