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

   "-----BEGIN CERTIFICATE-----\n"
   "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\n"
   "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\n"
   "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\n"
   "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\n"
   "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n"
   "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\n"
   "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\n"
   "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\n"
   "tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\n"
   "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\n"
   "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\n"
   "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\n"
   "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\n"
   "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\n"
   "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\n"
   "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\n"
   "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\n"
   "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\n"
   "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\n"
   "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\n"
   "-----END CERTIFICATE-----\n"

Note the ``\n`` at the end of each line.

See the comprehensive `tutorial on SSL.com`_ for instructions on how to convert between different certificate formats and encodings.


API documentation
*****************

| Header file: :file:`include/modem/modem_key_mgmt.h`
| Source files: :file:`lib/modem_key_mgmt/`

.. doxygengroup:: modem_key_mgmt
   :project: nrf
   :members:
