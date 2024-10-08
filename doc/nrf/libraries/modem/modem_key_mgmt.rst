.. _modem_key_mgmt:

Modem key management
####################

.. contents::
   :local:
   :depth: 2

The modem key management library provides functions to manage the credentials stored in the nRF91 Series LTE modem.
The library uses credential storage management to add, update, and delete credentials using the ``%CMNG`` command.
See the `Credential storage management %CMNG`_ section in the nRF9160 AT Commands Reference Guide or the `same section <nRF91x1 credential storage management %CMNG_>`_ in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.

Each set of keys and certificates that is stored in the modem is identified by a security tag (``sec_tag``).
You specify this tag when adding the credentials and use it when you update or delete them.
All related credentials share the same security tag.
You can use the library to check if a specific security tag exists.

To establish a connection, pass the security tag to the :ref:`nrfxlib:nrf_modem` when creating a secure socket.

See :ref:`nrfxlib:security_tags` for more information about how security tags are used in the Modem library.

.. _cert_dwload:

Certificates
************

You can download a certificate for a given server using your web browser.
Alternatively, you can obtain it from a dedicated website like `SSL Labs`_.

Certificates come in different formats.
To provision the certificate to the nRF91 Series DK, it must be in PEM format.
The PEM format looks like this::

   -----BEGIN CERTIFICATE-----
   MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G
   A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp
   Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1
   MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG
   A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI
   hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL
   v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8
   eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq
   tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd
   C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa
   zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB
   mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH
   V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n
   bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG
   3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs
   J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO
   291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS
   ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd
   AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7
   TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==
   -----END CERTIFICATE-----

See the comprehensive `tutorial on SSL.com`_ for instructions on how to convert between different certificate formats and encodings.

Usage
*****

The different types of credentials supported by the library are defined by the :c:enum:`modem_key_mgmt_cred_type` enum.

The following code snippet shows how to write a CA chain certificate to the modem:

.. code-block:: c

   int err;
   nrf_sec_tag_t sec_tag = 42;
   static const char cert[] = {
           #include "YourCert.pem.inc"
   };

   err = modem_key_mgmt_write(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert, sizeof(cert));
   if (err) {
           printk("Failed to provision certificate, err %d\n", err);
   }

The following code snippet shows how to check if a CA chain certificate exists in the modem:

.. code-block:: c

   int err;
   nrf_sec_tag_t sec_tag = 42;
   bool exists;

   err = modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
   if (err) {
           printk("Failed to check if credential exists\n");
           return;
   }

   if (exists) {
           printk("Credential exists in the modem\n");
   } else {
           printk("Credential does not exist in the modem\n");
   }

The following code snippet shows how to check if the CA chain certificate stored in the modem is the same as another CA chain certificate:

.. code-block:: c

   int mismatch;
   nrf_sec_tag_t sec_tag = 42;
   static const char cert[] = {
           #include "YourCert.pem.inc"
   };

   mismatch = modem_key_mgmt_cmp(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert, sizeof(cert));
   if (mismatch) {
           printk("Certificate mismatch\n");
   } else {
           printk("Certificate match\n");
   }

The following code snippet shows how to read a CA chain certificate stored in the modem:

.. code-block:: c

   int err;
   nrf_sec_tag_t sec_tag = 42;
   char cert[CERT_SIZE];
   size_t len;

   len = sizeof(cert);

   err = modem_key_mgmt_read(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert, &len);
   if (err) {
           printk("Failed to read certificate\n");
   }

The following code snippet shows how to delete a CA chain certificate stored in the modem:

.. code-block:: c

   int err;
   nrf_sec_tag_t sec_tag = 42;

   err = modem_key_mgmt_delete(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
   if (err) {
           printk("Failed to delete existing certificate, err %d\n", err);
   }

The following code snippet shows how to delete all credentials associated with a security tag in the modem:

.. code-block:: c

   int err;
   nrf_sec_tag_t sec_tag = 42;

   err = modem_key_mgmt_clear(sec_tag);
   if (err) {
           printk("Failed to clear credentials on sectag, err %d\n", err);
   }

API documentation
*****************

| Header file: :file:`include/modem/modem_key_mgmt.h`
| Source files: :file:`lib/modem_key_mgmt/`

.. doxygengroup:: modem_key_mgmt
