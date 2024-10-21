.. _ug_nrf70_developing_wifi_psa_support:

Wi-Fi PSA support
#################

.. contents::
   :local:
   :depth: 2

The nRF70 Series device Wi-Fi solution supports Platform Security Architecture(PSA) security framework.
The nRF70 Series device Wi-Fi solution achieves this by providing PSA API support for Wi-Fi cryptographic operations.

.. _ug_nrf70_developing_enabling_psa_support:

Enabling Wi-Fi PSA support
**************************

To enable the Wi-Fi PSA support in your applications, you must enable the :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ALT_NCS_PSA` Kconfig option in your application.

.. _ug_nrf70_developing_current_psa_support:

Current Wi-Fi PSA Support level
*******************************

The nRF70 Series device Wi-Fi solution currently supports WPA2 and open security profiles. WPA2 security profiles use PSA APIs while performing cryptographic operations during connection establishment.
WPA3 and Enterprise security profiles currently are disabled for PSA operation and will not function when enabling :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ALT_NCS_PSA` kconfig option.

.. _ug_nrf70_developing_connection_establishment:

Connection establishement for WPA2 using PSA
********************************************

There is no change in the command used in establishing Wi-Fi connection.
The below command can be used to establish a Wi-Fi connection using WPA2 security profile.

.. code-block:: console
   uart:~$ wifi connect -s <SSID> -p <passphrase> -k 1
