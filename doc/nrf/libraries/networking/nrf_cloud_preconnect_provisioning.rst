.. _lib_nrf_cloud_preconnect_provisioning:

nRF Cloud preconnect provisioning
#################################

.. contents::
   :local:
   :depth: 2

This page describes preconnect provisioning for devices without a Nordic cellular modem, such as nRF71 Series and nRF70 Series Wi-Fi devices.
Preconnect provisioning installs the credentials on the device and registers the device with your nRF Cloud account before the device connects to nRF Cloud for the first time, using the scripts from the `nRF Cloud Utils`_ Python package.
These devices store the credentials in the :ref:`TLS credentials <zephyr:sockets_tls_credentials_subsys>` subsystem (:kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_MGMT_TLS_CRED`).

For devices with a Nordic cellular modem, such as nRF91 Series devices, use the :ref:`lib_nrf_provisioning` service for over-the-air provisioning.

Overview
********

Connections to nRF Cloud are mutually authenticated: the device authenticates the server, and the server authenticates the device.
To achieve this, two steps are necessary:

* Provisioning - Installs the following credentials on the device:

  * The nRF Cloud root CA certificate.
  * The device credentials, which are a private key and a device certificate signed by your own CA certificate.

* Onboarding - Registers the device certificate with your nRF Cloud account.

These two steps involve the following certificates, each with a different role:

* Your own CA certificate - Signs the device certificates.
  It stays on your host, and is not installed on the device or uploaded to nRF Cloud.
* The nRF Cloud root CA certificate - Signs the nRF Cloud server certificates.
  It is installed on the device during provisioning, so that the device can check the authenticity of the server.
* The device certificate - Signed by your own CA certificate.
  It is installed on the device during provisioning and registered with nRF Cloud during onboarding, so that the server can check the authenticity of the device.

The device certificate is issued for a device ID, which must match the device ID that the firmware uses to connect to nRF Cloud (see :ref:`configuration_device_id`).
Devices without a modem have no modem UUID or IMEI that the ``device_credentials_installer`` script can read, so pass the device ID with its ``--id-str`` option.

.. _nrf_cloud_preconnect_prerequisites:

Prerequisites
*************

Before you provision your first device, install nRF Cloud Utils as described in its `README <nRF Cloud Utils_>`_.
Create your own CA certificate with the ``create_ca_cert`` script, setting ``-c`` to your two-letter country code:

.. code-block:: console

   create_ca_cert -c US -f self_

This creates the :file:`self_*_ca.pem` certificate and the :file:`self_*_prv.pem` private key used in the following sections.

.. _nrf_cloud_preconnect_provisioning:

Provisioning
************

You can provision the credentials in one of the following ways, listed from most to least secure:

* :ref:`nrf_cloud_preconnect_keygen` (recommended) - The device generates its private key, which never leaves the device.
* :ref:`nrf_cloud_preconnect_host_key` - The private key is generated on the host then installed on the device.
* :ref:`nrf_cloud_preconnect_hardcoded` - The private key is built into the firmware. For testing only.

All methods create an :file:`onboard.csv` file that you use to :ref:`onboard <nrf_cloud_preconnect_onboarding>` the devices.
Add the ``--coap`` option to the provisioning command if the device connects to nRF Cloud using CoAP, so that the CoAP root CA certificate is also installed.

With the two methods that use the ``device_credentials_installer`` script, the device already runs its firmware, so replace *device_id* in the command with the device ID that the firmware uses.
The library logs the device ID when it initializes (:kconfig:option:`CONFIG_NRF_CLOUD_PRINT_DETAILS`, enabled by default):

.. code-block:: console

   <inf> nrf_cloud_info: Device ID: <device_id>

.. _nrf_cloud_preconnect_keygen:

On-device key generation
========================

The device generates its private key and certificate signing request (CSR) using the :ref:`lib_nrf_cloud_credentials_keygen` library.
The ``device_credentials_installer`` script reads the CSR over the shell, signs the device certificate with your CA certificate, and installs it on the device.

The firmware must enable the :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN` and :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_SHELL` Kconfig options.
The ``--verify`` option, which checks that the installed device certificate matches the on-device key, also requires the :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY` Kconfig option.

To provision the device, connect it to your host and run the following command:

.. parsed-literal::
   :class: highlight

   device_credentials_installer --ca self_*_ca.pem --ca-key self_*_prv.pem --id-str *device_id* -s -d --cmd-type nrf_cloud_cred_shell --verify

See `nRF Cloud Utils: Devices without a modem <nRF Cloud Utils devices without a modem_>`_ for the other options of the script.

.. _nrf_cloud_preconnect_host_key:

Host-generated key
==================

The ``device_credentials_installer`` script generates the private key and the device certificate on the host, and installs them using the :ref:`TLS Credentials Shell <zephyr:tls_credentials_shell>`.
The firmware must enable the :kconfig:option:`CONFIG_TLS_CREDENTIALS_SHELL` Kconfig option.
It must also disable the :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN` Kconfig option, if enabled.
Otherwise, the library uses the on-device key instead of the installed private key.

To provision the device, connect it to your host and run the following command:

.. parsed-literal::
   :class: highlight

   device_credentials_installer --ca self_*_ca.pem --ca-key self_*_prv.pem --id-str *device_id* -s -d --cmd-type tls_cred_shell --local-cert

This method is less secure than :ref:`nrf_cloud_preconnect_keygen`: the private key exists on the host, and it is stored on the device as raw key material rather than as a non-exportable key.
Delete the private key from the host after the installation.
See `nRF Cloud Utils: Devices without a modem <nRF Cloud Utils devices without a modem_>`_ for the other options of the script.

.. _nrf_cloud_preconnect_hardcoded:

Hard-coded credentials
======================

With the :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option, the library installs credentials that are built into the firmware when it initializes.

.. caution::
   With this option, the device private key is kept in unprotected memory.
   Because this is insecure, use it for testing purposes only.

The credentials are included from the header file set by the :kconfig:option:`CONFIG_NRF_CLOUD_CERTIFICATES_FILE` Kconfig option.
The default header, :file:`subsys/net/lib/nrf_cloud/common/include/nrf_cloud-certs.h`, includes the :file:`ca-cert.pem`, :file:`client-cert.pem`, and :file:`private-key.pem` files from the include path.

To use these credentials, complete the following steps:

1. Choose a globally unique `device ID <nRF Cloud Device ID_>`_ that is not prefixed with ``nrf-``.
#. Generate the credentials with the ``create_device_credentials`` script, setting ``-c`` to your two-letter country code:

   .. parsed-literal::
      :class: highlight

      create_device_credentials --ca self_*_ca.pem --ca-key self_*_prv.pem -c US --cn *device_id* -p dev_credentials --embed-save --csv onboard.csv

   The ``--embed-save`` option saves the three :file:`.pem` files in the :file:`dev_credentials` folder.
   See `Create Device Credentials <nRF Cloud Utils Create Device Credentials_>`_ for more details.
#. Add the folder containing the files to the include path of your application, for example with ``zephyr_include_directories(certs)`` in your :file:`CMakeLists.txt` file.
   Do not place them in :file:`subsys/net/lib/nrf_cloud/common/include`, as files in that folder take precedence.
#. Set the following Kconfig options in your application configuration, for example in the :file:`prj.conf` file, replacing *device_id* with the device ID:

   .. parsed-literal::
      :class: highlight

      CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES=y
      CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN=n
      CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_SHELL=n
      CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME=y
      CONFIG_NRF_CLOUD_CLIENT_ID="*device_id*"

   On-device key generation must be disabled, as it would make the library use the on-device key instead of the hard-coded private key.
   The device ID must be the common name of the certificate.
#. Build and program the firmware.

.. _nrf_cloud_preconnect_onboarding:

Onboarding
**********

Devices must be `onboarded <nRF Cloud Onboarding_>`_ to your nRF Cloud account before they can connect.
Onboarding uses the :file:`onboard.csv` file created during :ref:`provisioning <nrf_cloud_preconnect_provisioning>`, which can contain one or more devices.

To onboard several devices at once, provision each of them with the ``--append`` option of the ``device_credentials_installer`` script, which adds a row for each device to the same :file:`onboard.csv` file.
The ``create_device_credentials`` script always appends to the file.

You can onboard the devices in one of the following ways.

Onboarding with the nrf_cloud_onboard script
============================================

To onboard the devices from the command line, run the ``nrf_cloud_onboard`` script with the :file:`onboard.csv` file:

.. parsed-literal::
   :class: highlight

   nrf_cloud_onboard --api-key *your_api_key* --csv onboard.csv

Where *your_api_key* is your legacy nRF Cloud API key, see `nRF Cloud Utils requirements`_.
See `nRF Cloud Utils onboarding`_ for more details.

Onboarding with the nRF Cloud portal
====================================

To onboard the devices from the nRF Cloud portal, go to the **Devices** section, select **Add devices**, and follow the instructions.
Supply your :file:`onboard.csv` file when prompted.
See `nRF Cloud Utils onboarding`_ for the CSV format, if you need to create the file manually.
