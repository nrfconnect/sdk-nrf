Blutooth stack architecture
###########################

The Bluetooth protocol has a layered architecture, and in |NCS| the Bluetooth implementation combines
components from the Zephyr Project and |NCS| components developed by Nordic Semiconductor. This section
briefly describes the main components and provides links to pages with detailed information.

The following diagram shows the main components of a Bluetooth application with focus on the protocol stack.


Radio (PHY)
***********

The Bluetooth Low Energy radio (PHY layer) supports 4 different modes with different bandwidth and range:

* S8 Coded PHY, 125kbps, longest range, optional support
* S2 Coded PHY, 500kbps, optional support
* 1 Mbps, Mandatory support
* 2 Mbps, shortest range, optional support

Key features to support Bluetooth Direction Finding is also part of the PHY layer.

Feature support for PHY features depend on specific SoCs. Please refer to the Bluetooth feature support
table in the :ref:`software_maturity` page.

Controller
**********

The SoftDevice Controller is the supported Bluetooth controller in the |NCS|. Application examples in the SDK
use the SoftDevice Controller as the default controller, and it is the controller used for release testing of the SDK.
The Bluetooth support includes:

* Implementation of new Bluetooth LE controller features.
* Valid qualification (QDID) for each SDK release.

The Zephyr Project has a community support Bluetooth LE Controller. This Bluetooth controller will be the default when
sing Zephyr RTOS stand-alone. It is possible to configure projects in |NCS| to use the Zephyr Controller, but
Nordic Semiconductor do not intend to support this configuration for production.

For more information on Bluetooth Controller see here: :ref:`ug_ble_controller`

Zephyr Host
***********

|NCS| use the Bluetooth Host implementation in the Zephyr project for host feature support. The Zephyr Host
implementation is tested with the rest of the SDK for releases, and a valid QDID is provided for each SDK release.

Documentation on the Zephyr Host is found here: :ref:`bluetooth_le_host` (Zephyr)
