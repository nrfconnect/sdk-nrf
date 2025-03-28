.. _ug_wifi_enterprise_mode:

Enterprise mode
################

Enterprise mode for Wi-Fi® is typically used in business environments or larger networks which require enhanced security, centralized management of users by utilizing Public Key Infrastructure (PKI).

Prerequisites
=============

* **RADIUS Server**: Along with self-signed local certificate(s) and private key for both Server-Side and Client-Side (for EAP-TLS)
* **Wi-Fi® Access Point**: Which supports Enterprise Mode.
* **nRF70 Series device** : With certificates for Enterprise Mode available at zephyr/samples/net/wifi/test_certs.


RADIUS Server Configuration
===========================

Hostapd is an open-source user space software which provides an integrated RADIUS server which can be used to simplify the setup for Enterprise mode.
Hence, in this example, we use Hostapd as RADIUS server (Authentication Server) to verify Enterprise mode functionality with 7002 DK along the commercial or test Access Points as Authenticator.

Hostapd Installation
---------------------
.. code-block:: console

    git clone git://w1.fi/hostap.git

    cd hostap/hostapd

    cp defconfig .config

Edit the .config file for Hostapd to use it as RADIUS server

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
.. code-block:: console

    make clean ; make

Copy the certificates for EAP-TLS to the hostapd folder

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
    # Enable eap_server when we use hostapd integrated EAP server instead of external RADIUS authentication
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
----------------
Assuming eno1 is the Laptop interface connected to AP (Authenticator) via Ethernet

.. code-block:: bash

    ./hostapd -i eno1 tls.conf

    or

    To enable debug messages and Key data
    ./hostapd -i eno1 tls.conf -ddK


Wi-Fi® Access Point Configuration
=================================
Configure an Access Point with Authentication method as WPA2-Enterprise

Server IP Address - IP of the RADIUS (Hostapd) Server

Server Port - 1812

Connection Secret - whatever

PMF - Capable

Apply the Configurations

Build the nRF70 series DK for shell sample with Enterprise Mode
=================================================================

Verify that the Client-Side Certificates required for EAP-TLS are available

.. code-block:: bash

    ls -l zephyr/samples/net/wifi/test_certs

    cd nrf/samples/wifi/shell

    west build -p -b nrf7002dk/nrf5340/cpuapp -- -DEXTRA_CONF_FILE=overlay-enterprise.conf -DCONFIG_WIFI_NM_WPA_SUPPLICANT_LOG_LEVEL_DBG=y -DCONFIG_LOG_MODE_IMMEDIATE=y

    west flash

To connect to WPA3-Enterprise AP
---------------------------------

.. code-block:: console

    wifi connect -s <SSID> -k 7 -a anon -K whatever -S 2 -w 2

example:

.. code-block:: console

    wifi connect -s WPA3-ENT_ZEPHYR_5 -k 7 -a anon -K whatever -S 2 -w 2


To connect the DK to WPA2-Enterprise AP
---------------------------------------

.. code-block:: console

    wifi connect -s <SSID> -k 7 -a anon -K whatever

example:

.. code-block:: console

    wifi connect -s WPA2-ENT_ZEPHYR_2 -k 7 -a anon -K whatever
