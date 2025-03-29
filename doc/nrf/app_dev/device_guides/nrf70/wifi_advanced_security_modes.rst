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

See `Zephyr Wi-Fi management`_ for more information on how to configure and use the Wi-Fi enterprise security mode.


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

To enable the nRF70 PSA crypto support in your applications, use the :kconfig:option:`CONFIG_HOSTAP_CRYPTO_ALT_PSA` Kconfig option.

The Wi-Fi connection process is similar to the non-PSA mode, however, the only difference is that the cryptographic operations are performed using PSA crypto APIs.
