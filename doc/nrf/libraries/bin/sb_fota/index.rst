.. _libsb_fota_readme:

Softbank FOTA library
#####################

.. contents::
   :local:
   :depth: 2

Softbank FOTA library is a modem :term:`Firmware Over-the-Air (FOTA) <Firmware Over-the-Air (FOTA) update>` library for Softbank carrier network.
All M2M devices that uses Softbank network must use this library.

This library is not intended to be a general purpose FOTA library.

Usage
*****

To use this library, the application must set the Kconfig option :kconfig:`CONFIG_SB_FOTA` to ``y`` and then register a callback for the library using :c:func:`sb_fota_init` function.
This is demonstrated in the following example and it is based on the assumption that you are initializing the :ref:`nrf_modem` and LTE connection when the device boots:

.. code-block:: c

    static void modem_fota_callback(enum sb_fota_event e)
    {
       switch(e) {
       case FOTA_EVENT_DOWNLOADING:
          printk("FOTA_EVENT_DOWNLOADING\n");
          break;
       case FOTA_EVENT_IDLE:
          printk("FOTA_EVENT_IDLE\n");
          break;
       case FOTA_EVENT_MODEM_SHUTDOWN:
          printk("FOTA_EVENT_MODEM_SHUTDOWN\n");
          break;
       case FOTA_EVENT_REBOOT_PENDING:
          printk("FOTA_EVENT_REBOOT_PENDING\n");
          sys_reboot(SYS_REBOOT_COLD);
          break;
       }
    }

    void main(void)
    {
       if (sb_fota_init(&modem_fota_callback) != 0) {
          printk("Failed to initialize modem FOTA\n");
          return;
       }
       ...
    }


To initialize the :ref:`nrf_modem` and LTE connection automatically, set the following Kconfig options in the :file:`prj.conf` file in the application folder:

* :kconfig:`CONFIG_NRF_MODEM_LIB` to ``y``
* :kconfig:`CONFIG_NRF_MODEM_LIB_SYS_INIT` to ``y``
* :kconfig:`CONFIG_LTE_LINK_CONTROL` to ``y``
* :kconfig:`CONFIG_LTE_AUTO_INIT_AND_CONNECT` to ``y``
* :kconfig:`CONFIG_SB_FOTA` to ``y``

In some cases, your application might require controlling the LTE link manually.

Requirements and limitations
============================

The application can control the modem usage normally, like any application based on the |NCS|, but with the following limitations:

* Occasionally, the library might need to connect to `nRF Cloud`_ to check and possibly download a new modem firmware image.
  It issues the :c:enum:`FOTA_EVENT_DOWNLOADING` event when it starts the download.
  At that time, application must not use any TLS sockets that are using offloaded TLS stack from the modem. 
  Also, it is recommended to stop all network operations until the :c:enum:`FOTA_EVENT_IDLE` event is received, as there might be mandatory operation mode switches between NB-IoT and LTE networks.

* When the modem is updated with a new firmware, it gets disconnected from the network and shuts down.
  This is indicated by the :c:enum:`FOTA_EVENT_MODEM_SHUTDOWN` event.
  The modem update can take a few minutes.
  After the modem is updated, it requires the device to boot itself, so that it can return all modem libraries into a known state.
  This request is indicated by the :c:enum:`FOTA_EVENT_REBOOT_PENDING` event.
  However, if no event handler is specified, the device reboots automatically.

To prevent any automatic reboots, it is recommended to install an event handler with :c:func:`sb_fota_init` API.

Configuration options
*********************

* :kconfig:`CONFIG_SB_FOTA` - Enables the Softbank FOTA library
* :kconfig:`CONFIG_SB_FOTA_AUTOINIT` - Initializes the library automatically
* :kconfig:`CONFIG_SB_FOTA_TLS_SECURITY_TAG` - Security tag (``sec_tag``) for nRF Cloud TLS connection
* :kconfig:`CONFIG_SB_FOTA_JWT_SECURITY_TAG` - ``sec_tag`` for authentication with the cloud.

Provisioning
************

Before using Softbank FOTA library with `nRF Cloud`_, you must provision the device.
This library requires two security tags for the following purposes:

* For securing a TLS connection with nRF Cloud
* For identifying a device

Modem firmware v1.3.0 allows a device to issue `JSON Web Tokens (JWT) <JSON Web Token (JWT)_>`_ at run time that are signed with a certain public and private key pair.
This key pair must be generated beforehand, and the public key must be provisioned to the nRF Cloud.

Provisioning by certificate
===========================

Currently, nRF Cloud does not allow provisioning with only a public key.
Hence you must provision a certificate using `ProvisionDevices API`_.

For  provisioning the certificates, first generate a Certificate Signing Request (CSR) with the modem.
For more information, see the documentation on `AT%KEYGEN set command`_.
In the following example, ``sec_tag 51`` is used, so the :kconfig:`CONFIG_SB_FOTA_JWT_SECURITY_TAG` must be set accordingly:

.. code-block:: none

   AT%KEYGEN=51,2,0
   %KEYGEN: "MIIBCzCBrwIBADAvMS0wKwYDVQQDDCQ1MDM2MzE1NC0zOTMwLTR...Tq1mTwUAIjrLiJYwk5MdrlslxGsXOxkk0Z5S-kg"
   OK

Note that the response is in the form ``Base64Url(CSR_DER).Base64Url(cose_sign)`` and only the first part (``Base64Url(CSR_DER)``) is required.

After base64url decoding, a PEM encoded CSR is obtained.

   .. code-block:: none

      -----BEGIN CERTIFICATE REQUEST-----
    MIIBCzCBrwIBADAvMS0wKwYDVQQDDCQ1MDM2MzE1NC0zOTMwLTRkOTYtODBmOS0x
    MzEyMWNmMzVhZGMwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAR4k6fuuSFZBCuD
    67fGZ2EENXpW+dmgiOjf2M6adTwOESbWClwyfaZBlz3yvANkjOarTz4lggxynBbe
    u9xHHYACoB4wHAYJKoZIhvcNAQkOMQ8wDTALBgNVHQ8EBAMCA+gwDAYIKoZIzj0E
    AwIFAANJADBGAiEA5tuNCs3OzBuWSyjUs27yELGOlUt77LEmgXXcA1RUaF4CIQDf
    XLwnpIpMUH6Oj4fPWqNCuk/3d+VLgcYLnaYELwdZbw==
    -----END CERTIFICATE REQUEST-----

A customer can create or manage certificates in different ways.
In the following example, CA is assumed to be used and the certification process is demonstrated with self-signed certificates and OpenSSL command line tool.
CSR generated from the AT%KEYGEN AT command is stored in :file:`example.csr`.

First, create a CA certificate with a signing key and sign a certificate with it:

.. code-block:: sh

    openssl genrsa -out ca.key 2048
    openssl req -new -x509 -key ca.key -out ca.crt
    openssl x509 -req -in example.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out example.crt

Once the certificate is generated from the given CSR, this data can be provisioned to nRF Cloud.
See `ProvisionDevices API`_ for details on the :file:`CSV` file format.

.. code-block:: sh

    # Extract the device UUID from certificate
    openssl x509 -in example.crt -noout -text | awk '/Subject: CN/{print $4",,,\""}' > provision.csv

    # Append the certificate
    cat example.crt >>provision.csv

    # complete the data line
    echo "\"" >> provision.csv

    # Push data to cloud
    curl -X POST https://api.nrfcloud.com/v1/devices --data-binary @provision.csv -H "Content-Type: application/octet-stream" -H "Authorization: Bearer 7b0d6e0f.....f31f18"

You can now use the device.

.. note::
   As the device uses only JWT and a keypair to authenticate, you need not provision that certificate back to the device.
   Similarly, cloud will extract the public key from a certificate when it verifies the JWT.

API documentation
*****************

| Header files: :file:`lib/bin/sb_fota/include`
| Source files: :file:`lib/bin/sb_fota`

Softbank FOTA library API
=========================

.. doxygengroup:: sb_fota
   :project: nrf
   :members:

Softbank FOTA OS layer
======================

.. doxygengroup:: sb_fota_os
   :project: nrf
   :members:
