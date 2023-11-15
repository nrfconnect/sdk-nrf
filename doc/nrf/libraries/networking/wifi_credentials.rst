.. _lib_wifi_credentials:

Wi-Fi credentials
#################

.. contents::
   :local:
   :depth: 2

The Wi-Fi credentials library provides means to load and store Wi-Fi network credentials.

Overview
********

This library uses either Zephyr's settings subsystem or Platform Security Architecture (PSA) protected storage to store credentials.
It also holds a list of SSIDs in RAM to provide dictionary-like access using SSIDs as keys.

Configuration
*************

To use the Wi-Fi credentials library, enable the :kconfig:option:`CONFIG_WIFI_CREDENTIALS` Kconfig option.

You can pick the backend using the following options:

* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_BACKEND_PSA` - Default option for non-secure targets, which includes a TF-M partition (non-minimal TF-M profile type).
* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_BACKEND_SETTINGS` - Default option for secure targets.

To configure the maximum number of networks, use the :kconfig:option:`CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES` Kconfig option.

The IEEE 802.11 standard does not specify the maximum length of SAE passwords.
To change the default, use the :kconfig:option:`CONFIG_WIFI_CREDENTIALS_SAE_PASSWORD_LENGTH` Kconfig option.

When using the PSA protected storage backend, entries will be saved under consecutive UIDs with an offset configured with the :kconfig:option:`CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET` Kconfig option.

Adding credentials
******************

You can add credentials using the :c:func:`wifi_credentials_set_personal` and :c:func:`wifi_credentials_set_personal_struct` functions.
The former will build the internally used struct from given fields, while the latter takes the struct directly.
If you add credentials with the same SSID twice, the older entry will be overwritten.

Querying credentials
********************

With an SSID, you can query credentials using the :c:func:`wifi_credentials_get_by_ssid_personal` and :c:func:`wifi_credentials_get_by_ssid_personal_struct` functions.

You can iterate over all stored credentials with the :c:func:`wifi_credentials_for_each_ssid` function.
Deleting or overwriting credentials while iterating is allowed, since these operations do not change internal indices.

Removing credentials
********************

You can remove credentials using the :c:func:`wifi_credentials_delete_by_ssid` function.

Limitations
***********

The library has the following limitations:

* Although permitted by the IEEE 802.11 standard, this library does not support zero-length SSIDs.
* Wi-Fi Protected Access (WPA) Enterprise credentials are not yet supported.
* The number of networks stored is fixed compile time.

API documentation
*****************

| Header file: :file:`include/net/wifi_credentials.h`
| Source files: :file:`subsys/net/lib/wifi_credentials`

.. doxygengroup:: wifi_credentials
   :project: nrf
   :members:
