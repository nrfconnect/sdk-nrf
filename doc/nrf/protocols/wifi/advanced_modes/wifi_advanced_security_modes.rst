.. _ug_nrf70_wifi_advanced_security_modes:

Wi-Fi Enterprise test: X.509 Certificate header generation
**********************************************************

Wi-Fi enterprise security requires use of X.509 certificates, test certificates
in PEM format are committed to the repo at :zephyr_file:`samples/net/wifi/test_certs/` and the during the
build process the certificates are converted to a C header file that is included by the Wi-Fi shell
module or the :ref:`lib_wifi_credentials` library.

.. code-block:: bash

    $ cp client.pem ${ZEPHYR_BASE}/samples/net/wifi/test_certs/
    $ cp client-key.pem ${ZEPHYR_BASE}/samples/net/wifi/test_certs/
    $ cp ca.pem ${ZEPHYR_BASE}/samples/net/wifi/test_certs/
    $ cp client.pem ${ZEPHYR_BASE}/samples/net/wifi/test_certs/client2.pem
    $ cp client-key.pem ${ZEPHYR_BASE}/samples/net/wifi/test_certs/client-key2.pem
    $ cp ca.pem ${ZEPHYR_BASE}/samples/net/wifi/test_certs/ca2.pem
    $ west build -p -b <board> samples/net/wifi -- -DEXTRA_CONF_FILE=overlay-enterprise.conf

.. note::
     The EAP phase2 certificates (suffixed with 2) are unused in ``EAP-TLS`` but are mandatory for building the sample application.
     The phase1 certificates are copied as phase2 certificates to avoid build errors.

To initiate Wi-Fi connection, the following command can be used:

.. code-block:: console

    uart:~$ wifi connect -s <SSID> -k 7 -a anon -K <key passphrase>

Server certificate is also provided in the same directory for testing purposes.
Any AAA server can be used for testing purposes, for example, ``FreeRADIUS`` or ``hostapd``.

.. note::

    The certificates are for testing purposes only and should not be used in production.


Wi-Fi PSA support
#################

The nRF70 Series device Wi-Fi solution supports `Platform Security Architecture (PSA)`_ (PSA) APIs for cryptographic operations.

The nRF70 Series device Wi-Fi solution currently supports only WPA2-personal security profile in PSA mode.
WPA3-personal and Enterprise security profiles will be supported in future releases using PSA APIs.

Enabling Wi-Fi PSA support
**************************

To enable the Wi-Fi PSA support in your applications, use the :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ALT_NCS_PSA` Kconfig option.

Wi-Fi connection process is similar to the non-PSA mode, the only difference is that the cryptographic operations are performed using PSA APIs.
