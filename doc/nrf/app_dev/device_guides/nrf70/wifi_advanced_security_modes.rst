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

See :ref:`wifi_mgmt` for more information on how to configure and use the Wi-Fi enterprise security mode.


.. _ug_nrf70_wifi_enterprise_mode:

Enterprise mode testing on linux using hostapd
==============================================

Enterprise mode for Wi-Fi is used in business environments or larger networks, which require enhanced security and centralized management of users by utilizing Public Key Infrastructure (PKI).

Prerequisites
-------------

To use this mode, ensure that the following prerequisites are met:

* RADIUS server in addition to self-signed local certificates and private key for both server-side and client-side (for EAP-TLS).
* Wi-Fi Access Point (AP) that supports Enterprise mode.
* nRF70 Series device with certificates for Enterprise mode available in the :file:`zephyr/samples/net/wifi/test_certs` folder.

RADIUS server configuration
---------------------------

Hostapd is an open-source user space software that provides an integrated RADIUS server, which can be used to simplify the setup for Enterprise mode.
Therefore, in the following example, hostapd is used as a RADIUS server (authentication server) to verify Enterprise mode functionality with the nRF7002 DK, along with commercial or test access points as the Authenticator.

Hostapd installation
--------------------

To  install hostapd, complete the following steps:

1. Install hostapd by using the following commands:

   .. code-block:: console

      git clone git://w1.fi/hostap.git

      cd hostap/hostapd

      cp defconfig .config

#. Edit the :file:`.config` file for hostapd to use it as a RADIUS server by using the following commands:

   .. code-block:: console

      Comment (by adding #) the following configurations
      #CONFIG_DRIVER_HOSTAP=y
      #CONFIG_DRIVER_NL80211=y
      #CONFIG_LIBNL32=y

      Enable the following configurations (by removing # from the front)
      CONFIG_DRIVER_NONE=y
      CONFIG_RADIUS_SERVER=y
      CONFIG_EAP_PSK=y
      CONFIG_EAP_PWD=y
      CONFIG_EAP_GPSK_SHA256=y
      CONFIG_EAP_FAST=y

      Add the following configurations
      CONFIG_PEERKEY=y
      CONFIG_IEEE80211W=y

      Verify required EAP Types are enabled
      "CONFIG_EAP=y"
      "CONFIG_EAP_TLS=y"
      "CONFIG_EAP_PEAP=y"
      "CONFIG_EAP_TTLS=y"

Build the hostapd executable
----------------------------

To  build the hostapd executable, complete the following steps:

1. Build the hostapd  executable by using the following commands:

   .. code-block:: console

      make clean ; make

#. Copy the certificates for EAP-TLS to the hostapd folder by using the following commands:

   .. code-block:: bash

      cp zephyr/samples/net/wifi/test_certs/*  hostap/hostapd/

      touch hostapd.eap_user_tls

      vim hostapd.eap_user_tls

      $ cat hostapd.eap_user_tls
      # Phase 1 users
      *       TLS

      touch tls.conf

      vim tls.conf

      $ cat tls.conf
      # Building hostapd as a standalone RADIUS server
      driver=none
      # RADIUS clients configuration
      radius_server_clients=hostapd.radius_clients
      radius_server_auth_port=1812
      # Enable eap_server when using hostapd integrated EAP server instead of external RADIUS authentication
      eap_server=1
      # EAP server user database
      eap_user_file=hostapd.eap_user_tls
      # CA certificate
      ca_cert=ca.pem
      # Server certificate
      server_cert=server.pem
      # Private key matching with the server certificate
      private_key=server-key.pem
      # Passphrase for private key
      private_key_passwd=whatever
      logger_syslog=-1
      logger_syslog_level=2
      logger_stdout=-1
      logger_stdout_level=2
      ctrl_interface=/var/run/hostapd
      ctrl_interface_group=0

      vim hostapd.radius_clients

      $ cat hostapd.radius_clients
      RADIUS client configuration for the RADIUS server
      0.0.0.0/0       whatever

Run the hostapd
---------------

Run hostapd by using the following commands, assuming that **eno1** is the laptop interface connected to the AP (Authenticator) through Ethernet.

.. code-block:: bash

   ./hostapd -i eno1 tls.conf

   #To enable debug messages and Key data
   ./hostapd -i eno1 tls.conf -ddK


Wi-Fi access point configuration
---------------------------------

Configure an access point with WPA2-Enterprise authentication method using the following parameters:

* Server IP address - IP address of the RADIUS (hostapd) server
* Server port - 1812
* Connection secret - whatever
* Protected Management Frames (PMF) - Capable (for WPA2-Enterprise), Required (for WPA3-Enterprise)

Build the nRF70 Series DK for Shell sample with Enterprise mode
----------------------------------------------------------------

To build the nRF70 Series DK for the :ref:`wifi_shell_sample` sample with Enterprise mode, complete the following steps:

1. Verify that the client-side certificates required for EAP-TLS are available by using the following commands:

   .. code-block:: bash

      ls -l zephyr/samples/net/wifi/test_certs

      cd nrf/samples/wifi/shell

      west build -p -b nrf7002dk/nrf5340/cpuapp -S shell_SNIPPET=wifi-enterprise -- -DCONFIG_WIFI_NM_WPA_SUPPLICANT_LOG_LEVEL_DBG=y -DCONFIG_LOG_MODE_IMMEDIATE=y

      west flash

#. Connect to the WPA3-Enterprise AP by using the following commands:

   .. code-block:: console

      wifi connect -s <SSID> -k 7 -a anon -K whatever -S 2 -w 2

   Example:

   .. code-block:: console

      wifi connect -s WPA3-ENT_ZEPHYR_5 -k 7 -a anon -K whatever -S 2 -w 2

#. Connect the DK to the WPA2-Enterprise AP by using the following command:

   .. code-block:: console

      wifi connect -s <SSID> -k 7 -a anon -K whatever

   Example:

   .. code-block:: console

      wifi connect -s WPA2-ENT_ZEPHYR_2 -k 7 -a anon -K whatever

.. _ug_nrf70_developing_wifi_psa_support:

Platform Security Architecture (PSA) crypto support
***************************************************

The nRF70 Series devices support the `Platform Security Architecture (PSA)`_ security framework.
This framework provides a set of APIs for cryptographic operations, which are used by the nRF70 Series.
This improves the security of the nRF70 device compared to the non-PSA mode.

.. note::

      Currently, the PSA crypto support is only applicable to the WPA2™ and WPA3™-personal security profiles.

Enable PSA support
==================

To enable the nRF70 PSA crypto support in your applications, use the :kconfig:option:`CONFIG_HOSTAP_CRYPTO_ALT_PSA` Kconfig option.

The Wi-Fi connection process is similar to the non-PSA mode, however, the only difference is that the cryptographic operations are performed using PSA crypto APIs.

WPA3-SAE support
----------------

To enable WPA3-SAE support in your applications, use the :kconfig:option:`CONFIG_HOSTAP_CRYPTO_WPA3_PSA` Kconfig option.
This is disabled by default.
