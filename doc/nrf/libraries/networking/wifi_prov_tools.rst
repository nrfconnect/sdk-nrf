.. _wifi_prov_tools:

Wi-Fi Provisioning Configuration Generator
##########################################

.. contents::
   :local:
   :depth: 2

This tool generates Protocol Buffers (protobuf) configuration messages for Wi-Fi® provisioning, supporting both EAP-TLS (Enterprise) and Personal (WPA2-PSK/WPA3-PSK) modes.

Prerequisites
*************

The script automatically generates its own protobuf dependencies when needed.
No manual setup is required.

.. note::
   The script requires the ``protoc`` compiler to be available in your PATH.
   This is typically included in the NCS toolchain.

Usage
*****

The script supports two main modes:

Enterprise mode
===============

Run the following command for enterprise networks using certificate-based authentication, EAP-TLS is used as an example:

.. code-block:: bash

   python3 generate_wifi_prov_config.py "MyWi-Fi" "AA:BB:CC:DD:EE:FF" \
       -d /path/to/certs \
       -j -o eap_tls_config.json

The following are the required certificate files in the cert directory:

* :file:`ca.pem` - CA certificate
* :file:`client.pem` - Client certificate
* :file:`client-key.pem` - Client private key
* :file:`ca2.pem` - Secondary CA certificate
* :file:`client2.pem` - Secondary client certificate
* :file:`client-key2.pem` - Secondary client private key

Personal mode
=============

Run the following commands for home or office networks using passphrase authentication:

.. code-block:: bash

   # WPA2-PSK
   python3 generate_wifi_prov_config.py "MyWi-Fi" "AA:BB:CC:DD:EE:FF" \
       -w "mypassword" -a 3 -j -o personal_config.json

   # WPA3-SAE
   python3 generate_wifi_prov_config.py "MyWi-Fi" "AA:BB:CC:DD:EE:FF" \
       -w "mypassword" -a 10 -j -o wpa3_config.json

Command line options
====================

.. list-table:: Command-line arguments
   :header-rows: 1
   :widths: 20 40 40

   * - Argument
     - Description
     - Default values and notes

   * - ``ssid``
     - Wi-Fi network name
     - (positional)

   * - ``bssid``
     - Wi-Fi BSSID (MAC address)
     - (positional)

   * - ``-d, --cert-dir``
     - Certificate directory (for EAP-TLS mode)
     - (optional)

   * - ``-w, --passphrase``
     - Wi-Fi passphrase (for Personal mode)
     - (optional)

   * - ``-a, --auth-mode``
     - Authentication mode
     - 0=OPEN, 1=WEP, 2=WPA_PSK, 3=WPA2_PSK, 4=WPA_WPA2_PSK, 5=WPA2_ENTERPRISE, 6=WPA3_PSK, 7=NONE, 8=PSK, 9=PSK_SHA256, 10=SAE, 11=WAPI, 12=EAP, 13=WPA_AUTO_PERSONAL, 14=DPP, 15=EAP_PEAP_MSCHAPV2, 16=EAP_PEAP_GTC, 17=EAP_TTLS_MSCHAPV2, 18=EAP_PEAP_TLS, 19=FT_PSK, 20=FT_SAE, 21=FT_EAP, 22=FT_EAP_SHA384, 23=SAE_EXT_KEY; default: 5=WPA2_ENTERPRISE

   * - ``-c, --channel``
     - Wi-Fi channel
     - default: 0

   * - ``-b, --band``
     - Wi-Fi band
     - 0=AUTO, 1=2.4GHz, 2=5GHz; default: 0

   * - ``-i, --identity``
     - EAP identity
     - default: user@example.com

   * - ``-p, --password``
     - EAP password
     - default: user_password

   * - ``-k, --private-key-passwd``
     - Primary private key password
     - (optional)

   * - ``-k2, --private-key-passwd2``
     - Secondary private key password
     - (optional)

   * - ``-o, --output``
     - Output file
     - (optional)

   * - ``-j, --json``
     - Display JSON configuration
     - (optional)

Output
******

The tool always displays the encoded protobuf format:

Encoded Protobuf (Base64) - Base64-encoded protobuf string for transmission

When using the ``-j`` flag, JSON configuration is also displayed for reference.

When using the ``-o`` option, files are saved in the specified format:

* ``-j -o file.json`` - Save as JSON file
* ``-o file.bin`` - Save as binary protobuf file

Configuration details
*********************

The generated configuration includes:

* *SSID* - Wi-Fi network name
* *BSSID* - Access point MAC address
* *Channel* - Wi-Fi channel number
* *Band* - 2.4GHz or 5GHz
* *Authentication Mode* - WPA-PSK, WPA2-PSK, WPA2-ENTERPRISE, WPA3-PSK
* *Mode* - EAP-TLS (Enterprise) or Personal
* *Identity* - EAP identity (for Enterprise mode)
* *Passphrase* - Wi-Fi password (for Personal mode, masked in output)

Error handling
**************

The tool performs the following validation checks:

* Certificate directory existence (for EAP-TLS mode)
* Required certificate files presence
* Valid authentication mode selection
* Proper parameter combinations

The following are common error messages you may encounter while using the tool:

* "Certificate directory does not exist"
* "Missing required certificate files"
* "Must specify either --cert-dir (EAP-TLS) or --passphrase (Personal)"
* "Auth mode X is not valid for EAP-TLS" (for invalid EAP auth modes)

Integration
***********

The generated protobuf messages can be used with the following systems:

* Nordic Semiconductor's Wi-Fi provisioning service
* Zephyr RTOS Wi-Fi stack
* Custom Wi-Fi provisioning applications

The binary protobuf format is suitable for transmission over Bluetooth LE or other communication channels.
