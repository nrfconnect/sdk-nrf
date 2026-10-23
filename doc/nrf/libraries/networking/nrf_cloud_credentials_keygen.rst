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

1. If you do not already have one, create your own CA certificate and key (for example, using the :file:`create_ca_cert.py` script from nRF Cloud Utils) to sign your device certificates.
#. Generate the device key on the device.
#. Generate a CSR signed by the device key and transfer it off the device over the shell.
#. Sign the device certificate locally with your CA certificate.
   nRF Cloud does not issue the device certificate.
#. Onboard the device to your nRF Cloud account by uploading the device certificate in a CSV file.
#. Upload your self-signed CA certificate and the AWS root CA certificate (for CoAP, the nRF Cloud CoAP server root CA certificate) to the device.

Configuration
*************

Set the following Kconfig option to enable the feature:

* :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN` - Makes the library use the device private key generated on the device.
  It enables the TLS credentials key generation and the TLS credentials shell.

The option is experimental.
It depends on the TLS credentials management option (:kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_MGMT_TLS_CRED`) and the TLS credentials protected storage backend (:kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE`).
It cannot be used together with the :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option.

Generate the key for the nRF Cloud security tag (:kconfig:option:`CONFIG_NRF_CLOUD_SEC_TAG`, or :kconfig:option:`CONFIG_NRF_CLOUD_COAP_SEC_TAG` for CoAP).
The security tag cannot be changed at run time with this option, so the :c:func:`nrf_cloud_sec_tag_set` and :c:func:`nrf_cloud_sec_tag_coap_jwt_set` functions are not available.
A key generated for one security tag would not be found after a change to another security tag.

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

.. _lib_nrf_cloud_credentials_keygen_onboarding:

Onboarding a device with the credentials installer
***************************************************

The :file:`device_credentials_installer.py` script from the nRF Cloud Utils package can drive the whole on-device key flow over the shell using the ``tls_cred_shell`` command type.
Without the ``--local-cert`` option, the device generates the key and CSR itself, and the private key never leaves the device.

If you do not already have a CA certificate and private key, use the :file:`create_ca_cert.py` script to create a CA and a key that you can use to sign all your device certificates:

.. code-block:: console

   create_ca_cert -f self_

This generates the :file:`self_*_ca.pem` and :file:`self_*_prv.pem` files used in the following commands.

Run the appropriate command for the transport in use, replacing *device_id* with your device ID.
The ``--verify`` option confirms that the installed device certificate matches the on-device key (see the explanation that follows).

.. tabs::

   .. tab:: MQTT

      .. code-block:: console

         device_credentials_installer --ca self_*_ca.pem --ca-key self_*_prv.pem --id-str device_id -s -d --cmd-type tls_cred_shell --verify

   .. tab:: CoAP

      .. code-block:: console

         device_credentials_installer --ca self_*_ca.pem --ca-key self_*_prv.pem --id-str device_id -s -d --cmd-type tls_cred_shell --verify --coap

The ``--coap`` option installs the CoAP root CA certificate required for the CoAP transport.
The command generates an :file:`onboard.csv` file.
Complete the onboarding by adding the device to your nRF Cloud account, for example, using the ``nrf_cloud_onboard`` script:

.. parsed-literal::
   :class: highlight

   nrf_cloud_onboard --api-key *your_api_key* --csv onboard.csv

Because the private key is non-exportable, ``--verify`` cannot compare key hashes.
Instead, the installer reads the device public key (with the ``cred pubkey`` command) and compares it against the public key in the installed device certificate.
If you do not need verification, omit the ``--verify`` option.

.. note::
   Do not pass ``--local-cert`` or ``--local-cert-file`` for an on-device key.
   With these options, the installer generates the key on the host instead.
