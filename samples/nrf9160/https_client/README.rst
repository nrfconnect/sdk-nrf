.. _https_client:

nRF9160: HTTPS Client
#####################

The HTTPS Client sample demonstrates a minimal implementation of HTTP communication.
It shows how to set up a TLS session towards an HTTPS server and how to send an HTTP request.

Overview
********

The sample first initializes the :ref:`nrfxlib:bsdlib` and AT communications.
Next, it provisions a root CA certificate to the modem using the :ref:`modem_key_mgmt` library.
Provisioning must be done before connecting to the LTE network, because the certificates can only be provisioned when the device is not connected.

The sample then establishes a connection to the LTE network, sets up the necessary TLS socket options, and connects to an HTTPS server.
It sends an HTTP HEAD request and prints the response code in the terminal.

Obtaining a certificate
=======================

The sample connects to ``www.google.com``, which requires an X.509 certificate.
This certificate is provided in the :file:`samples/nrf9160/https_client/cert` folder.

To connect to other servers, you might need to provision a different certificate.
You can download a certificate for a given server using your web browser.
Alternatively, you can obtain it from a dedicated website like `SSL Labs`_.

Certificates come in different formats.
To provision the certificate to the nRF9160 DK, it must be in PEM format.
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


Requirements
************

* The following development board:

  * |nRF9160DK|

* .. include:: /includes/spm.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/https_client`

.. include:: /includes/build_and_run_nrf9160.txt


Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the sample starts, provisions certificates, connects to the LTE network and to google.com, and then sends an HTTP HEAD request.
#. Observe that the HTTP HEAD request returns ``HTTP/1.1 200 OK``.

Sample Output
=============

The sample shows the following output:

.. code-block:: console

   HTTPS client sample started
   Provisioning certificate
   Waiting for network.. OK
   Connecting to google.com
   Sent 64 bytes
   Received 903 bytes

   >        HTTP/1.1 200 OK

   Finished, closing socket.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`at_cmd_readme`
  * :ref:`at_notif_readme`
  * :ref:`modem_key_mgmt`
  * ``lib/lte_link_control``

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

In addition, it uses the following samples:

From |NCS|
  * :ref:`secure_partition_manager`
