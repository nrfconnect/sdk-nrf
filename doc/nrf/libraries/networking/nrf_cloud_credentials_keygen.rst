.. _lib_nrf_cloud_credentials_keygen:

nRF Cloud on-device key generation
###################################

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

1. If you do not already have one, create your own CA certificate and key (for example, using the :file:`create_ca_cert.py` script from nRF Cloud Utils) to sign your device certificates.
#. Generate the device key on the device.
#. Generate a CSR signed by the device key and transfer it off the device (over the shell, or using the API).
#. Sign the device certificate locally with your CA certificate.
   nRF Cloud does not issue the device certificate.
#. Onboard the device to your nRF Cloud account by uploading the device public key in a CSV file.
#. Upload your self-signed CA certificate and the AWS root CA certificate (for CoAP, the nRF Cloud CoAP server root CA certificate) to the device.

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

The :file:`device_credentials_installer.py` script from the nRF Cloud Utils package can drive the whole on-device key flow over the shell using the ``nrf_cloud_cred_shell`` command type.
With this command type, the device generates the key and CSR itself, so the ``--local-cert`` option (required by the ``tls_cred_shell`` command type) is not used, and the private key never leaves the device.

If you do not already have a CA certificate and private key, use the :file:`create_ca_cert.py` script to create a CA and a key that you can use to sign all your device certificates:

.. code-block:: console

   create_ca_cert -f self_

This generates the :file:`self_*_ca.pem` and :file:`self_*_prv.pem` files used in the following commands.

Run the appropriate command for the transport in use, replacing *device_id* with your device ID.
The ``--verify`` option confirms that the installed device certificate matches the on-device key (see the explanation that follows).

.. tabs::

   .. tab:: MQTT

      .. code-block:: console

         device_credentials_installer --ca self_*_ca.pem --ca-key self_*_prv.pem --id-str device_id -s -d --cmd-type nrf_cloud_cred_shell --verify

   .. tab:: CoAP

      .. code-block:: console

         device_credentials_installer --ca self_*_ca.pem --ca-key self_*_prv.pem --id-str device_id -s -d --cmd-type nrf_cloud_cred_shell --verify --coap

The ``--coap`` option installs the CoAP root CA certificate required for the CoAP transport.
The command generates an :file:`onboard.csv` file.
Complete the onboarding by adding the device to your nRF Cloud account, for example using the ``nrf_cloud_onboard`` script:

.. parsed-literal::
   :class: highlight

   nrf_cloud_onboard --api-key *your_api_key* --csv onboard.csv

Because the private key is non-exportable, ``--verify`` cannot compare key hashes.
Instead, the installer reads the device public key (with the ``pubkey`` command) and compares it against the public key in the installed device certificate.
This requires the :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY` Kconfig option to be enabled on the device.
If you do not need verification, omit the ``--verify`` option.

.. note::
   Do not pass ``--local-cert`` or ``--local-cert-file`` with the ``nrf_cloud_cred_shell`` command type.
   The key and CSR are generated on the device.
   The installer rejects this combination to prevent accidentally generating a key on the host.

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_credentials_keygen.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/common/src/`

.. doxygengroup:: nrf_cloud_credentials_keygen
