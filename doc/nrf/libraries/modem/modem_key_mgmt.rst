.. _modem_key_mgmt:

Modem key management
####################

.. contents::
   :local:
   :depth: 2

The modem key management library provides functions to manage the credentials stored in the nRF91 Series LTE modem.
The library uses credential storage management to add, update, and delete credentials using the ``%CMNG`` command.
See the `Credential storage management %CMNG`_ section in the nRF9160 AT Commands Reference Guide or the same section in the `nRF91x1 AT Commands Reference Guide`_ depending on the SiP you are using.

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


API documentation
*****************

| Header file: :file:`include/modem/modem_key_mgmt.h`
| Source files: :file:`lib/modem_key_mgmt/`

.. doxygengroup:: modem_key_mgmt
   :project: nrf
   :members:
