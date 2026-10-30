.. _lib_nrf_cloud_credentials_keygen:

nRF Cloud on-device key generation
##################################

.. contents::
   :local:
   :depth: 2

The nRF Cloud on-device key generation feature lets a device generate its own TLS private key and a Certificate Signing Request (CSR) without the key ever leaving the device.
It is part of the :ref:`lib_nrf_cloud` library and is intended for the manual provisioning flow on devices that do not have a modem-managed key store.

Overview
********

The private key is generated using the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` as a persistent, non-exportable ECC P-256 key, stored using the :ref:`Secure Storage <secure_storage_in_ncs>`.
The key is referenced for TLS by its PSA key ID through the ``TLS_CREDENTIAL_PRIVATE_KEY_PSA`` credential type, so the private key is never exported to the application or written to the TLS credentials store as raw key material.

A typical provisioning flow is as follows:

1. Generate the device key on the device.
#. Generate a CSR signed by the device key and transfer it off the device (over the shell, or using the API).
#. Sign the device certificate on the host with your own CA certificate.
   nRF Cloud does not issue the device certificate.
#. Install the device certificate and the nRF Cloud root CA certificates on the device.
#. Onboard the device to your nRF Cloud account by uploading the device certificate.

See `nRF Cloud Utils: Devices without a modem <nRF Cloud Utils devices without a modem_>`_ for the complete provisioning and onboarding process.

The device key is re-registered with the TLS credentials module from the Secure Storage during nRF Cloud initialization, so a key generated in a previous session is available for the TLS handshake after a reboot.

Configuration
*************

Set the following Kconfig options to enable the feature:

* :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN` - Enables on-device key and CSR generation and the associated API.
* :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY` - Enables exporting the on-device public key so host tooling can verify that the key matches the device certificate.
   Enabled by default, disable to save code if verification is not needed.
* :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_SHELL` - Enables the ``nrf_cloud_cred`` shell commands.

The :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN` and :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_SHELL` options are experimental and depend on the TLS credentials management option (:kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_MGMT_TLS_CRED`).

Shell commands
**************

When the :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_SHELL` Kconfig option is enabled, the following commands are available under ``nrf_cloud_cred``.
If the security tag is omitted, the nRF Cloud security tag (:kconfig:option:`CONFIG_NRF_CLOUD_SEC_TAG`) is used.

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Command
     - Description
   * - ``nrf_cloud_cred keygen [<sec_tag>]``
     - Generate the device private key. Fails if a key already exists for the security tag.
   * - ``nrf_cloud_cred csr [<sec_tag>] [<subject>]``
     - Generate and print a Base64-encoded DER CSR signed by the device key. If the subject is omitted, ``CN=<device id>`` is used.
   * - ``nrf_cloud_cred delete [<sec_tag>]``
     - Delete the device private key.
   * - ``nrf_cloud_cred pubkey [<sec_tag>]``
     - Export and print the Base64-encoded public key (requires :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY`).

To rotate the key, delete it and generate a new one.
The previously issued device certificate no longer matches the new key and must be re-provisioned.

.. _lib_nrf_cloud_credentials_keygen_onboarding:

Onboarding a device with the credentials installer
***************************************************

The ``device_credentials_installer`` script from `nRF Cloud Utils`_ can perform this flow over the shell using the ``nrf_cloud_cred_shell`` command type, which requires the :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_SHELL` Kconfig option.

Because the private key is non-exportable, the ``--verify`` option of the script cannot compare key hashes.
Instead, the script reads the device public key with the ``pubkey`` command and compares it against the public key in the installed device certificate.
This requires the :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY` Kconfig option.

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_credentials_keygen.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/common/src/`

.. doxygengroup:: nrf_cloud_credentials_keygen
