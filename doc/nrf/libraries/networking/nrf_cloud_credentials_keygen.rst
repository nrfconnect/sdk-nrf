.. _lib_nrf_cloud_credentials_keygen:

nRF Cloud on-device key generation
##################################

.. contents::
   :local:
   :depth: 2

The nRF Cloud on-device key generation feature lets the :ref:`lib_nrf_cloud` library connect with a TLS private key that the device generated itself, without the key ever leaving the device.
It is intended for the manual provisioning flow on devices that do not have a modem-managed key store.

Overview
********

The private key is generated with the TLS credentials key generation of Zephyr (:kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN`).
It is a persistent, non-exportable ECDSA P-256 key, created with the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` and stored using the :ref:`Secure Storage <secure_storage_in_ncs>`.
The key is registered for TLS by its PSA key ID through the ``TLS_CREDENTIAL_PRIVATE_KEY_PSA`` credential type, so the private key is never exported to the application or written to the TLS credentials store as raw key material.
The TLS credentials protected storage backend keeps the registration after a reboot.
The nRF Cloud library also signs its JSON Web Tokens with the key in PSA.

A typical provisioning flow is as follows:

1. Generate the device key on the device.
#. Generate a CSR signed by the device key and transfer it off the device over the shell.
#. Sign the device certificate on the host with your own CA certificate.
   nRF Cloud does not issue the device certificate.
#. Install the device certificate and the nRF Cloud root CA certificates on the device.
#. Onboard the device to your nRF Cloud account by uploading the device certificate.

For the complete provisioning and onboarding process, see :ref:`nrf_cloud_preconnect_keygen` and `nRF Cloud Utils: Devices without a modem <nRF Cloud Utils devices without a modem_>`_.

Configuration
*************

Set the following Kconfig option to enable the feature:

* :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN` - Makes the library use the device private key generated on the device.
  It enables the TLS credentials key generation and the TLS credentials shell.

The option is experimental.
It depends on the TLS credentials management option (:kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_MGMT_TLS_CRED`) and the TLS credentials protected storage backend (:kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE`).
It cannot be used together with the :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option.

Generate the key for the nRF Cloud security tag (:kconfig:option:`CONFIG_NRF_CLOUD_SEC_TAG`, or :kconfig:option:`CONFIG_NRF_CLOUD_COAP_SEC_TAG` for CoAP).

Shell commands
**************

The key generation commands of the :ref:`TLS credentials shell <zephyr:tls_credentials_shell>` generate the key and the CSR:

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Command
     - Description
   * - ``cred keygen <sec_tag>``
     - Generate the device private key. Fails if the security tag already has a private key.
   * - ``cred csr <sec_tag> <subject>``
     - Generate and print a Base64-encoded DER CSR signed by the device key. Use ``CN=<device id>`` as the subject.
   * - ``cred keydel <sec_tag>``
     - Delete the device private key.
   * - ``cred pubkey <sec_tag>``
     - Export and print the Base64-encoded public key.

To rotate the key, delete it and generate a new one.
The previously issued device certificate no longer matches the new key and must be re-provisioned.
