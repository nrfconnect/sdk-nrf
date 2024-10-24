.. _ug_nrf70_wifi_advanced_security_modes:

nRF70 Series advanced security modes
####################################

.. contents::
   :local:
   :depth: 2

Enterprise security
*******************

The nRF70 Series devices support Wi-Fi® enterprise security, which is a more secure form of Wi-Fi security compared to Wi-Fi personal security.
Wi-Fi enterprise security is used in corporate environments where the security requirements are more stringent.
It is based on the IEEE 802.1X standard, which defines the port-based network access control.

The nRF70 Series devices support the following Wi-Fi enterprise security mode, ``WPA2-EAP-TLS``.
This mode uses the Extensible Authentication Protocol (EAP) with Transport Layer Security (TLS) for authentication.
The client and the authentication server exchange certificates to authenticate each other.


Enterprise testing: X.509 certificate headers generation
========================================================

Wi-Fi enterprise security requires use of X.509 certificates.
Test certificates in PEM format are available at :zephyr_file:`samples/net/wifi/test_certs/` repository.
During the build process, the certificates are converted to a C header file that is included in the Wi-Fi shell module or the :ref:`Wi-Fi credentials <lib_wifi_credentials>` library.

To use custom certificates, use the following commands:

.. code-block:: bash

    $ cp client.pem samples/net/wifi/test_certs/
    $ cp client-key.pem samples/net/wifi/test_certs/
    $ cp ca.pem samples/net/wifi/test_certs/
    $ cp client.pem samples/net/wifi/test_certs/client2.pem
    $ cp client-key.pem samples/net/wifi/test_certs/client-key2.pem
    $ cp ca.pem samples/net/wifi/test_certs/ca2.pem

    $ west build -p -b <board> samples/net/wifi -- -DEXTRA_CONF_FILE=overlay-enterprise.conf

.. note::
     The EAP phase2 certificates (suffixed with 2) are unused in ``WPA2-EAP-TLS`` but are mandatory for building the sample application.
     The phase1 certificates are copied as phase2 certificates to avoid build errors as a temporary workaround.

To establish a Wi-Fi connection, use the following command:

.. code-block:: console

    uart:~$ wifi connect -s <SSID> -k 7 -a anon -K <key passphrase>

.. code-block:: console

   uart:~$ wifi_cred add -s <SSID> -k 7 -a anon -K <key passphrase>


.. note::

      The Wi-Fi credentials only support 16characters for the anonymous identity and the key passphrase.

The server certificate is also provided in the same directory for testing purposes.
You can use any AAA server for testing purposes, such as FreeRADIUS or hostapd.

.. note::

    The certificates are for testing purposes only and should not be used for production.

.. _ug_nrf70_developing_wifi_psa_support:

Platform Security Architecture (PSA) crypto support
***************************************************

The nRF70 Series devices support the `Platform Security Architecture (PSA)`_ security framework.
This framework provides a set of APIs for cryptographic operations, which are used by the nRF70 Series.
This improves the security of the nRF70 device compared to the non-PSA mode.

.. note::

      Currently, the PSA crypto support is only applicable to the WPA2™-personal security profile.

Enable PSA support
==================

To enable the nRF70 PSA crypto support in your applications, use the :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ALT_NCS_PSA` Kconfig option.

The Wi-Fi connection process is similar to the non-PSA mode, however, the only difference is that the cryptographic operations are performed using PSA crypto APIs.
