.. _wifi_provisioning_internal_sample:

Wi-Fi: Provisioning Internal
############################

.. contents::
   :local:
   :depth: 2

The Provisioning Internal sample demonstrates the internal Wi-Fi® provisioning functionality using the decoupled Wi-Fi provisioning library.
The sample demonstrates proper integration of the Wi-Fi provisioning library with comprehensive testing capabilities and human-readable protocol decoding.
It provides shell commands to test all supported Wi-Fi provisioning operations.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample uses the :ref:`lib_wifi_prov_core` library with a transport stub implementation that decodes and logs protobuf messages.
This allows testing of the provisioning protocol without requiring a Bluetooth® transport layer.

Features
********

This sample demonstrates the following features:

* Transport decoupling - Uses the Wi-Fi Provisioning Core library without Bluetooth dependencies.
* Protobuf decoding - Automatically decodes and logs all requests and responses.
* Comprehensive testing - Shell commands for all supported operations.
* Raw data support - Ability to send custom binary data for testing.
* Configurable generation - Wi-Fi configuration generated from Kconfig options.

Architecture
************

The sample demonstrates the decoupled architecture:

* Core Library - ``wifi_prov_core`` handles the protobuf protocol and business logic.
* Transport Stub - ``wifi_prov_transport_stub.c`` provides mock transport functions.
* Shell Interface - ``prov.c`` implements shell commands for testing.
* Configuration - Generated from Kconfig options at build time.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/provisioning/internal/Kconfig`):

.. _CONFIG_WIFI_PROV_CONFIG:

CONFIG_WIFI_PROV_CONFIG
   This option enables Wi-Fi provisioning.

.. _CONFIG_WIFI_PROV_SSID:

CONFIG_WIFI_PROV_SSID
   This option specifies the Wi-Fi SSID.

.. _CONFIG_WIFI_PROV_BSSID:

CONFIG_WIFI_PROV_BSSID
   This option specifies the Wi-Fi BSSID.

.. _CONFIG_WIFI_PROV_PASSPHRASE:

CONFIG_WIFI_PROV_PASSPHRASE
   This option specifies the Wi-Fi passphrase.

.. _CONFIG_WIFI_PROV_AUTH_MODE:

CONFIG_WIFI_PROV_AUTH_MODE
   This option specifies the Wi-Fi authentication mode.

.. _CONFIG_WIFI_PROV_CHANNEL:

CONFIG_WIFI_PROV_CHANNEL
   This option specifies the Wi-Fi channel.

.. _CONFIG_WIFI_PROV_BAND:

CONFIG_WIFI_PROV_BAND
   This option specifies the Wi-Fi band.

.. _CONFIG_WIFI_PROV_CERT_DIR:

CONFIG_WIFI_PROV_CERT_DIR
   This option specifies the Wi-Fi certificate directory.

.. _CONFIG_WIFI_PROV_PRIVATE_KEY_PASSWD:

CONFIG_WIFI_PROV_PRIVATE_KEY_PASSWD
   This option specifies the Wi-Fi private key password.

.. _CONFIG_WIFI_PROV_PRIVATE_KEY_PASSWD2:

CONFIG_WIFI_PROV_PRIVATE_KEY_PASSWD2
   This option specifies the Wi-Fi private key password.

.. _CONFIG_WIFI_PROV_IDENTITY:

CONFIG_WIFI_PROV_IDENTITY
   This option specifies the Wi-Fi identity.

.. _CONFIG_WIFI_PROV_PASSWORD:

CONFIG_WIFI_PROV_PASSWORD
   This option specifies the Wi-Fi password.

.. _CONFIG_WIFI_PROV_VOLATILE_MEMORY:

CONFIG_WIFI_PROV_VOLATILE_MEMORY
   This option specifies the Wi-Fi volatile memory.

.. _CONFIG_WIFI_PROV_SCAN_BAND:

CONFIG_WIFI_PROV_SCAN_BAND
   This option specifies the Wi-Fi scan band.

.. _CONFIG_WIFI_PROV_SCAN_PASSIVE:

CONFIG_WIFI_PROV_SCAN_PASSIVE
   This option specifies the Wi-Fi scan passive mode.

.. _CONFIG_WIFI_PROV_SCAN_PERIOD_MS:

CONFIG_WIFI_PROV_SCAN_PERIOD_MS
   This option specifies the Wi-Fi scan period in milliseconds.

.. _CONFIG_WIFI_PROV_SCAN_GROUP_CHANNELS:

CONFIG_WIFI_PROV_SCAN_GROUP_CHANNELS
   This option specifies the Wi-Fi scan group channels.

.. _CONFIG_WIFI_PROV_MAX_DATA_SIZE:

CONFIG_WIFI_PROV_MAX_DATA_SIZE
   This option specifies the maximum protobuf data size.

.. _CONFIG_WIFI_PROV_MAX_BASE64_SIZE:

CONFIG_WIFI_PROV_MAX_BASE64_SIZE
   This option specifies the maximum base64 input size.

.. _CONFIG_BT_WIFI_PROV_LOG_LEVEL:

CONFIG_BT_WIFI_PROV_LOG_LEVEL
   This option specifies the log level.


Authentication modes
********************

The sample supports the following authentication modes:

* 0: Open
* 1: WEP
* 2: WPA-PSK
* 3: WPA2-PSK
* 4: WPA3-SAE
* 5: WPA3-OWE
* 6: WPA2-Enterprise
* 7: WPA3-Enterprise
* 8: EAP-TLS
* 9: EAP-TTLS
* 10: EAP-PEAP
* 11: EAP-PWD
* 12: EAP-SIM
* 13: EAP-AKA
* 14: EAP-AKA'
* 15: EAP-FAST
* 16: EAP-TEAP
* 17: EAP-PAX
* 18: EAP-PSK
* 19: EAP-SAKE
* 20: EAP-IKEv2
* 21: EAP-GPSK
* 22: EAP-POTP
* 23: EAP-VENDOR

Wi-Fi bands
***********

The sample can perform Wi-Fi operations across the following bands:

* 1: 2.4 GHz
* 2: 5 GHz
* 3: 6 GHz

Shell commands
**************

The sample provides the following shell commands for testing Wi-Fi provisioning functionality:

.. list-table:: Shell commands for Wi-Fi provisioning
   :header-rows: 1

   * - Command
     - Description
   * - ``wifi_prov``
     - Send pregenerated Wi-Fi configuration data to the provisioning service.
   * - ``wifi_prov raw <base64_data>``
     - Send raw protobuf-encoded data (in Base64 format) to the provisioning service.
   * - ``wifi_prov dump_scan <base64_data>``
     - Decode and display scan results in a human-readable format from Base64 encoded protobuf data.
   * - ``wifi_prov get_status``
     - Send a ``GET_STATUS`` request to the provisioning service.
   * - ``wifi_prov start_scan``
     - Send a ``START_SCAN`` request to the provisioning service.
   * - ``wifi_prov stop_scan``
     - Send a ``STOP_SCAN`` request to the provisioning service.
   * - ``wifi_prov set_config``
     - Send a ``SET_CONFIG`` request with Wi-Fi configuration from Kconfig options.
   * - ``wifi_prov forget_config``
     - Send a ``FORGET_CONFIG`` request to the provisioning service.
   * - ``wifi_prov info``
     - Display information about the pregenerated Wi-Fi configuration data.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/provisioning/internal`

.. include:: /includes/build_and_run.txt

To build and run the sample, complete the following steps:

1. Configure the sample (optional) by running the following command:

   .. code-block:: bash

       # Edit prj.conf or use west build with -D options
       west build -b nrf7002dk/nrf5340/cpuapp samples/wifi/provisioning/internal \
           -- -DCONFIG_WIFI_PROV_SSID="MyNetwork" \
               -DCONFIG_WIFI_PROV_PASSPHRASE="mypassword"

#. Build the sample by running the following command:

   .. code-block:: bash

       west build -b nrf7002dk/nrf5340/cpuapp samples/wifi/provisioning/internal

#. Flash the device:

   .. code-block:: bash

       west flash

#. Connect to console and test:

   .. code-block:: bash

       # Connect to device console
       screen /dev/ttyACM0 115200

       # Test commands
       uart:~$ wifi_prov info
       uart:~$ wifi_prov get_status
       uart:~$ wifi_prov raw 0801

.. include:: /includes/wifi_refer_sample_yaml_file.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|

#. Run the following command to build and flash the sample:

   .. code-block:: bash

       west build -b nrf7002dk/nrf5340/cpuapp samples/wifi/provisioning/internal
       west flash

#. Connect to the device console and test basic commands:

   .. code-block:: text

       uart:~$ wifi_prov info
       Wi-Fi Configuration Information:
         Size: 156 bytes
         Data: 0x20000000
         First 16 bytes:
           0x08 0x04 0x12 0x0a 0x4d 0x79 0x57 0x69
           0x46 0x69 0x4e 0x65 0x74 0x77 0x6f 0x72

       uart:~$ wifi_prov get_status
       Getting Wi-Fi provisioning status...
       === Wi-Fi Provisioning Request ===
       Type: GET_STATUS
       ===============================
       Wi-Fi status request sent successfully

Advanced testing
================

Run the following command to test custom protobuf messages:

.. code-block:: text

    uart:~$ wifi_prov raw 0801
    Sending raw binary data to provisioning service...
    Binary data size: 2 bytes
    === Wi-Fi Provisioning Request ===
    Type: GET_STATUS
    ===============================
    Raw data sent successfully

Protocol testing
================

The sample automatically decodes and logs all protobuf messages:

.. code-block:: text

    uart:~$ wifi_prov start_scan
    Starting Wi-Fi scan...
    === Wi-Fi Provisioning Request ===
    Type: START_SCAN
    ===============================
    Wi-Fi scan started successfully

    # Response will be logged automatically:
    === Wi-Fi Provisioning Response ===
    Scan Result:
      Status: 0
      Networks found: 3
      Network 1:
        SSID: MyWi-FiNetwork
        BSSID: 11:22:33:44:55:66
        RSSI: -45
        Channel: 6
        Auth mode: 3

Directory structure
*******************

The directory structure of the sample is as follows:

.. code-block:: text

    samples/wifi/provisioning/internal/
    ├── CMakeLists.txt              # Build configuration
    ├── Kconfig                     # Kconfig options
    ├── prj.conf                    # Project configuration
    ├── README.rst                  # This documentation
    └── src/
        ├── main.c                  # Application entry point
        ├── prov.c                  # Shell command implementations
        └── wifi_prov_transport_stub.c  # Transport stub with decoding

Generated files
****************

During the build process, the following files are generated:

* :file:`wifi_config.h` - Header file with Wi-Fi configuration data declarations.
* :file:`wifi_config.c` - Source file with Wi-Fi configuration data definitions.
* Protobuf Python files - Generated from the :file:`*.proto` files for configuration.
