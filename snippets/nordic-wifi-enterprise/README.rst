.. _snippet-nordic-wifi-enterprise:

Nordic Wi-Fi Enterprise snippet (nordic-wifi-enterprise)
########################################################

.. contents::
   :local:
   :depth: 2

The ``nordic-wifi-enterprise`` snippet enables Wi-Fi® Enterprise security modes, which authenticate the device against a RADIUS server with certificates instead of a pre-shared key.

.. note::
   Use this snippet instead of Zephyr's ``wifi-enterprise`` snippet for Wi-Fi Enterprise builds in the |NCS|.

Overview
********

The snippet applies the following common configuration:

* Enables the enterprise crypto support in the WPA supplicant (:kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE`).
* Enables the TLS credentials subsystem (:kconfig:option:`CONFIG_TLS_CREDENTIALS`) and its shell (:kconfig:option:`CONFIG_TLS_CREDENTIALS_SHELL`), so that you can provision the certificates at runtime.
* Increases the Mbed TLS heap (:kconfig:option:`CONFIG_MBEDTLS_HEAP_SIZE`) and the credential buffer sizes that the certificate exchange requires.

For board targets that run on the application core, the snippet additionally enables the runtime certificate support in the Wi-Fi credentials library and the Wi-Fi shell, and increases the WPA supplicant thread stack size and the driver data heap size.
For the ``ns`` variants, it stores the credentials in protected storage (:kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE`) and enables link time optimization (:kconfig:option:`CONFIG_LTO`) to fit the larger image.

Usage
*****

Apply the snippet when building, for example:

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp -S nordic-wifi-enterprise

See also
********

* :ref:`ug_nrf70_wifi_advanced_security_modes` for the supported enterprise modes and the certificate provisioning procedure.
