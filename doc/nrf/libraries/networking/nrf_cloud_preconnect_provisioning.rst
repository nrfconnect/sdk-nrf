.. _lib_nrf_cloud_preconnect_provisioning:

nRF Cloud preconnect provisioning
#################################

.. contents::
   :local:
   :depth: 2

This page describes preconnect provisioning for devices without a Nordic cellular modem, such as nRF71 Series and nRF70 Series Wi-Fi® devices.
Preconnect provisioning installs the credentials on the device and registers the device with your nRF Cloud account before the device connects to nRF Cloud for the first time.
The process uses scripts from the `nRF Cloud Utils`_ Python package.
These devices store the credentials in the :ref:`TLS credentials <zephyr:sockets_tls_credentials_subsys>` subsystem (:kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_MGMT_TLS_CRED`).

For devices with a Nordic cellular modem, such as nRF91 Series devices, use the :ref:`lib_nrf_provisioning` service for over-the-air provisioning.

Overview
********

Connections to nRF Cloud are mutually authenticated.
The device authenticates the server, and the server authenticates the device.
To achieve this, two steps are necessary:

* Provisioning - Installs the following credentials on the device:

  * The nRF Cloud root CA certificate.
  * The device credentials, which are a private key and a device certificate signed by your own CA certificate.

* Onboarding - Registers the device certificate with your nRF Cloud account.

These two steps involve the following certificates, each with a different role:

* Your own CA certificate - Signs the device certificates.
  It stays on your host and is not installed on the device or uploaded to nRF Cloud.
* The nRF Cloud root CA certificate - Signs the nRF Cloud server certificates.
  It is installed on the device during provisioning, so that the device can check the authenticity of the server.
* The device certificate - Signed by your own CA certificate.
  It is installed on the device during provisioning and registered with nRF Cloud during onboarding, so that the server can check the authenticity of the device.

The device certificate is issued for a device ID, which must match the device ID that the firmware uses to connect to nRF Cloud (see :ref:`configuration_device_id`).
Devices without a modem have no modem UUID or IMEI that the ``device_credentials_installer`` script can read, so pass the device ID with its ``--id-str`` option.

.. _nrf_cloud_preconnect_prerequisites:

Prerequisites
*************

Before you provision your first device, complete the following steps:

1. Install nRF Cloud Utils as described in the `documentation <nRF Cloud Utils documentation_>`_.
#. To onboard the devices from the command line, also install the Memfault CLI, as described in the `nRF Cloud device onboarding quickstart`_.
#. Create your own CA certificate using the ``create_ca_cert`` script, setting ``-c`` to your two-letter country code:

   .. code-block:: console

      create_ca_cert -c US -f self_

   This creates the :file:`self_*_ca.pem` certificate and the :file:`self_*_prv.pem` private key used in the following sections.

.. _nrf_cloud_preconnect_provisioning:

Provisioning
************

You can provision the credentials in one of the following ways, listed from most to least secure:

* :ref:`nrf_cloud_preconnect_keygen` (recommended) - The device generates its private key, which never leaves the device.
* :ref:`nrf_cloud_preconnect_host_key` - The private key is generated on the host then installed on the device.
* :ref:`nrf_cloud_preconnect_hardcoded` - The private key is built into the firmware.
  This method is intended for testing purposes only.

All methods create an :file:`onboard.csv` file that you use to :ref:`onboard <nrf_cloud_preconnect_onboarding>` the devices.

.. _nrf_cloud_preconnect_inputs:

Inputs
======

Before you provision a device, determine the following inputs:

* Device ID - The device certificate is issued for the device ID that the firmware uses to connect to nRF Cloud.

  * With :ref:`nrf_cloud_preconnect_keygen` and :ref:`nrf_cloud_preconnect_host_key`, the firmware determines the device ID.
    The :ref:`lib_nrf_cloud` library logs it when it initializes (:kconfig:option:`CONFIG_NRF_CLOUD_PRINT_DETAILS`, enabled by default):

    .. code-block:: console

       <inf> nrf_cloud_info: Device ID: <device_id>

  * With :ref:`nrf_cloud_preconnect_hardcoded`, choose a globally unique `device ID <nRF Cloud Device ID_>`_ that is not prefixed with ``nrf-``, and set it in the :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID` Kconfig option.

* Transport - If the device connects to nRF Cloud using CoAP, add the ``--coap`` option to the provisioning command, so that the CoAP root CA certificate is also installed.
* Number of devices - To onboard several devices at once, add the ``--append`` option to the ``device_credentials_installer`` command, so that the script adds a row for each device to the same :file:`onboard.csv` file.
  The ``create_device_credentials`` script always appends to the file.

In the following sections, replace *device_id* with the device ID.

.. _nrf_cloud_preconnect_keygen:

On-device key generation
========================

In this method, the device generates its private key and certificate signing request (CSR) using the :ref:`lib_nrf_cloud_credentials_keygen` library.
The ``device_credentials_installer`` script reads the CSR over the shell, signs the device certificate with your CA certificate, and installs it on the device.

To provision the device, complete the following steps:

1. Set the following Kconfig options in your application configuration, for example in the :file:`prj.conf` file:

   .. parsed-literal::
      :class: highlight

      CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN=y
      CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_SHELL=y

#. Build and program the firmware, then read the device ID (see :ref:`nrf_cloud_preconnect_inputs`).
#. Connect the device to your host and run the following command:

   .. parsed-literal::
      :class: highlight

      device_credentials_installer --ca self_*_ca.pem --ca-key self_*_prv.pem --id-str *device_id* -s -d --cmd-type nrf_cloud_cred_shell [--verify] [--coap] [--append]

   The ``--verify`` option checks that the installed device certificate matches the on-device key.
   It requires the :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY` Kconfig option.
   See `nRF Cloud Utils: Devices without a modem <nRF Cloud Utils devices without a modem_>`_ for other script options.

.. _nrf_cloud_preconnect_host_key:

Host-generated key
==================

In this method, the ``device_credentials_installer`` script generates the private key and the device certificate on the host, and installs them using the :ref:`TLS Credentials Shell <zephyr:tls_credentials_shell>`.

This method is less secure than :ref:`nrf_cloud_preconnect_keygen` because the private key exists on the host and it is stored on the device as raw key material rather than as a non-exportable key.

To provision the device, complete the following steps:

1. Set the following Kconfig options in your application configuration, for example in the :file:`prj.conf` file:

   .. parsed-literal::
      :class: highlight

      CONFIG_TLS_CREDENTIALS_SHELL=y
      CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN=n

   Disable on-device key generation.
   Otherwise, the :ref:`lib_nrf_cloud` library uses the on-device key instead of the installed private key.
#. Build and program the firmware, then read the device ID (see :ref:`nrf_cloud_preconnect_inputs`).
#. Connect the device to your host and run the following command:

   .. parsed-literal::
      :class: highlight

      device_credentials_installer --ca self_*_ca.pem --ca-key self_*_prv.pem --id-str *device_id* -s -d --cmd-type tls_cred_shell --local-cert [--verify] [--coap] [--append]

   The ``--verify`` option checks that the installed credentials match the generated ones.
   See `nRF Cloud Utils: Devices without a modem <nRF Cloud Utils devices without a modem_>`_ for other script options.
#. Delete the private key from the host after the installation.

.. _nrf_cloud_preconnect_hardcoded:

Hard-coded credentials
======================

When the :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option is enabled, the :ref:`lib_nrf_cloud` library installs the credentials built into the firmware during initialization.

.. caution::
   With this option, the device private key is built into the firmware image as plain text.
   Because this is insecure, use it for testing purposes only.

The credentials are included from the header file set by the :kconfig:option:`CONFIG_NRF_CLOUD_CERTIFICATES_FILE` Kconfig option.
The default header, :file:`subsys/net/lib/nrf_cloud/common/include/nrf_cloud-certs.h`, includes the :file:`ca-cert.pem`, :file:`client-cert.pem`, and :file:`private-key.pem` files from the include path.

To provision the device, complete the following steps:

1. Set the following Kconfig options in your application configuration, for example in the :file:`prj.conf` file:

   .. parsed-literal::
      :class: highlight

      CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES=y
      CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN=n
      CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME=y
      CONFIG_NRF_CLOUD_CLIENT_ID="*device_id*"

   Disable on-device key generation.
   Otherwise, the nRF Cloud library uses the on-device key instead of the hard-coded private key.
#. Generate the credentials with the ``create_device_credentials`` script, setting ``-c`` to your two-letter country code:

   .. parsed-literal::
      :class: highlight

      create_device_credentials --ca self_*_ca.pem --ca-key self_*_prv.pem -c US --cn *device_id* -p dev_credentials --embed-save --csv onboard.csv [--coap]

   The device ID must be the common name of the certificate.
   The ``--embed-save`` option saves the three :file:`.pem` files in the :file:`dev_credentials` folder.
   See `Create Device Credentials <nRF Cloud Utils Create Device Credentials_>`_ for more details.
#. Add the folder containing the files to the include path of your application, for example by using ``zephyr_include_directories(certs)`` in your :file:`CMakeLists.txt` file.
   Do not place them in :file:`subsys/net/lib/nrf_cloud/common/include`, because files in that folder take precedence.
#. Build and program the firmware.

.. _nrf_cloud_preconnect_onboarding:

Onboarding
**********

Devices must be `onboarded <nRF Cloud Onboarding_>`_ to your nRF Cloud account before they can connect.
Onboarding uses the :file:`onboard.csv` file created during :ref:`provisioning <nrf_cloud_preconnect_provisioning>`, which can contain one or more devices (see :ref:`nrf_cloud_preconnect_inputs`).

You can onboard the devices in one of the following ways.

Onboarding with the nrfcloud CLI
================================

To onboard the devices from the command line, run the ``nrfcloud`` command of the Memfault CLI with the :file:`onboard.csv` file:

.. parsed-literal::
   :class: highlight

   nrfcloud --org-token $NRFCLOUD_OAT --org $NRFCLOUD_ORG --project $NRFCLOUD_PROJECT device create --csv onboard.csv

Where ``NRFCLOUD_OAT`` holds an organization auth token (OAT), and ``NRFCLOUD_ORG`` and ``NRFCLOUD_PROJECT`` the slugs of your nRF Cloud organization and project.
See the `nRF Cloud device onboarding prerequisites`_ for how to get these values.
Creating an OAT requires the Organization Admin role.
Store the token securely, as it grants organization admin access to the API.

Onboarding with the nRF Cloud portal
====================================

To onboard the devices from the nRF Cloud portal, go to the **Devices** section, select **Add devices**, and follow the instructions.
Supply your :file:`onboard.csv` file when prompted.
See `nRF Cloud Utils onboarding`_ for the CSV format, if you need to create the file manually.
