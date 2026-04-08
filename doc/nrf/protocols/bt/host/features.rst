.. _bluetooth-features:

Supported features
##################

.. contents::
    :local:
    :depth: 2

* Bluetooth Host

  * Generic Access Profile (GAP) with all possible LE roles

    * Peripheral & Central
    * Observer & Broadcaster
    * Multiple PHY support (2Mbit/s, Coded)
    * Extended Advertising
    * Periodic Advertising (including Sync Transfer)

  * GATT (Generic Attribute Profile)

    * Server (to be a sensor)
    * Client (to connect to sensors)
    * Enhanced ATT (EATT)
    * GATT Database Hash
    * GATT Multiple Notifications

  * Pairing support, including the Secure Connections feature from Bluetooth 4.2

  * Non-volatile storage support for permanent storage of Bluetooth-specific
    settings and data

  * Clean HCI driver abstraction

    * 3-Wire (H:5) & 5-Wire (H:4) UART
    * SPI
    * Local controller support as a virtual HCI driver

  * Isochronous channels
