WiFi Provisioning Configuration Generator
========================================

This tool generates protobuf configuration messages for WiFi provisioning, supporting both EAP-TLS (Enterprise) and Personal (WPA2-PSK/WPA3-PSK) modes.

Prerequisites
------------

1. Install protobuf compiler and Python dependencies:

   .. code-block:: bash

      pip install protobuf

2. Generate protobuf definitions:

   .. code-block:: bash

      cd subsys/bluetooth/services/wifi_prov/proto
      make generate

Usage
-----

The script supports two main modes:

EAP-TLS (Enterprise) Mode
~~~~~~~~~~~~~~~~~~~~~~~~~

For enterprise networks using certificate-based authentication:

.. code-block:: bash

   python3 generate_eap_tls_config.py "MyWiFi" "AA:BB:CC:DD:EE:FF" \
       -d /path/to/certs \
       -j -o eap_tls_config.json

Required certificate files in the cert directory:
- ``ca.pem`` - CA certificate
- ``client.pem`` - Client certificate
- ``client-key.pem`` - Client private key
- ``ca2.pem`` - Secondary CA certificate
- ``client2.pem`` - Secondary client certificate
- ``client-key2.pem`` - Secondary client private key

Personal Mode
~~~~~~~~~~~~

For home/office networks using passphrase authentication:

.. code-block:: bash

   # WPA2-PSK
   python3 generate_wifi_prov_config.py "MyWiFi" "AA:BB:CC:DD:EE:FF" \
       -w "mypassword" -a 3 -j -o personal_config.json

   # WPA3-PSK
   python3 generate_wifi_prov_config.py "MyWiFi" "AA:BB:CC:DD:EE:FF" \
       -w "mypassword" -a 6 -j -o wpa3_config.json

Command Line Options
-------------------

Positional Arguments:
- ``ssid`` - WiFi network name
- ``bssid`` - WiFi BSSID (MAC address)

Optional Arguments:
- ``-d, --cert-dir`` - Certificate directory (for EAP-TLS mode)
- ``-w, --passphrase`` - WiFi passphrase (for Personal mode)
- ``-a, --auth-mode`` - Authentication mode (0=NONE, 1=PSK, 2=PSK_SHA256, 3=SAE, 4=WAPI, 5=EAP, 6=WEP, 7=WPA_PSK, 8=WPA_AUTO_PERSONAL, 9=DPP, 10=EAP_PEAP_MSCHAPV2, 11=EAP_PEAP_GTC, 12=EAP_TTLS_MSCHAPV2, 13=EAP_PEAP_TLS, 14=FT_PSK, 15=FT_SAE, 16=FT_EAP, 17=FT_EAP_SHA384, 18=SAE_EXT_KEY, default: 5=EAP)
- ``-c, --channel`` - WiFi channel (default: 6)
- ``-b, --band`` - WiFi band (1=2.4GHz, 2=5GHz, default: 1)
- ``-i, --identity`` - EAP identity (default: user@example.com)
- ``-p, --password`` - EAP password (default: user_password)
- ``-o, --output`` - Output file (optional)
- ``-j, --json`` - Display JSON configuration (optional)

Examples
--------

Generate EAP-TLS Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Generate JSON configuration
   python3 generate_wifi_prov_config.py "MyWiFi" "AA:BB:CC:DD:EE:FF" \
       -d /path/to/certs \
       -j -o eap_tls_config.json

   # Generate binary protobuf with custom parameters
   python3 generate_wifi_prov_config.py "MyWiFi" "AA:BB:CC:DD:EE:FF" \
       -d /path/to/certs \
       -c 11 -b 2 -i "user@company.com" \
       -o config.bin

   # Generate base64 output to stdout
   python3 generate_wifi_prov_config.py "MyWiFi" "AA:BB:CC:DD:EE:FF" \
       -d /path/to/certs

Generate Personal Mode Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # WPA2-PSK configuration
   python3 generate_wifi_prov_config.py "MyWiFi" "AA:BB:CC:DD:EE:FF" \
       -w "mypassword" -a 3 -j -o personal_config.json

   # WPA3-PSK configuration
   python3 generate_wifi_prov_config.py "MyWiFi" "AA:BB:CC:DD:EE:FF" \
       -w "mypassword" -a 6 -j -o wpa3_config.json

   # 5GHz network with custom channel
   python3 generate_wifi_prov_config.py "MyWiFi" "AA:BB:CC:DD:EE:FF" \
       -w "mypassword" -a 3 -b 2 -c 36 \
       -j -o personal_5ghz_config.json

Complete Example Workflow
------------------------

1. Generate protobuf definitions:

   .. code-block:: bash

      cd subsys/bluetooth/services/wifi_prov/proto
      make generate

2. Generate EAP-TLS configuration:

   .. code-block:: bash

      CERT_DIR="/path/to/certificates"
      SSID="MyEnterpriseWiFi"
      BSSID="AA:BB:CC:DD:EE:FF"

      # Generate JSON configuration
      python3 generate_wifi_prov_config.py "$SSID" "$BSSID" \
          -d "$CERT_DIR" -j -o eap_tls_config.json

      # Generate binary protobuf
      python3 generate_wifi_prov_config.py "$SSID" "$BSSID" \
          -d "$CERT_DIR" -o eap_tls_config.bin

3. Generate Personal mode configuration:

   .. code-block:: bash

      SSID="MyHomeWiFi"
      BSSID="AA:BB:CC:DD:EE:FF"
      PASSPHRASE="mysecretpassword"

      # Generate Personal mode configuration
      python3 generate_wifi_prov_config.py "$SSID" "$BSSID" \
          -w "$PASSPHRASE" -a 3 -j -o personal_config.json

Output
------

The tool always displays the encoded protobuf format:

**Encoded Protobuf (Base64)** - Base64-encoded protobuf string for transmission

When using the ``-j`` flag, JSON configuration is also displayed for reference.

When using the ``-o`` option, files are saved in the specified format:

- ``-j -o file.json`` - Save as JSON file
- ``-o file.bin`` - Save as binary protobuf file

Configuration Details
-------------------

The generated configuration includes:

- **SSID**: WiFi network name
- **BSSID**: Access point MAC address
- **Channel**: WiFi channel number
- **Band**: 2.4GHz or 5GHz
- **Authentication Mode**: WPA-PSK, WPA2-PSK, WPA2-ENTERPRISE, WPA3-PSK
- **Mode**: EAP-TLS (Enterprise) or Personal
- **Identity**: EAP identity (for Enterprise mode)
- **Passphrase**: WiFi password (for Personal mode, masked in output)

Error Handling
-------------

The tool performs several validation checks:

- Certificate directory existence (for EAP-TLS mode)
- Required certificate files presence
- Valid authentication mode selection
- Proper parameter combinations

Common error messages:

- "Certificate directory does not exist"
- "Missing required certificate files"
- "Must specify either --cert-dir (EAP-TLS) or --passphrase (Personal)"

Integration
----------

The generated protobuf messages can be used with:

- Nordic Semiconductor's WiFi provisioning service
- Zephyr RTOS WiFi stack
- Custom WiFi provisioning applications

The binary protobuf format is suitable for transmission over Bluetooth LE or other communication channels.
