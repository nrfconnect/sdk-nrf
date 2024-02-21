.. _lib_azure_iot_hub:

Azure IoT Hub
#############

.. contents::
   :local:
   :depth: 2

The Azure IoT Hub library provides an API to connect to an `Azure IoT Hub`_ instance and interact with it.
It connects to Azure IoT Hub using MQTT over TLS.

Optionally, the library supports `Azure IoT Hub Device Provisioning Service (DPS)`_.
DPS can be enabled at compile time to make use of the device provisioning services for onboarding of devices to Azure IoT Hub.
When the device provisioning is complete, the library automatically connects to the assigned Azure IoT Hub.

The library also has integrated support for a proprietary FOTA solution.
For more information on Azure FOTA, see the documentation on :ref:`lib_azure_fota` library and :ref:`azure_iot_hub` sample.

The library uses `Azure SDK for Embedded C`_ for message processing and other operations.
For more information on how Azure SDK for Embedded C is integrated in this library, see :module_file:`Azure SDK for Embedded C IoT client libraries<azure-sdk-for-c: sdk/docs/iot/README.md>`.

.. important::
   If the server sends a device-bound message when the device is unavailable for a period of time, for instance while in LTE Power Saving Mode, the server will most likely terminate the TCP connection.
   This will result in additional data traffic as the device has to reconnect to the server, which in turn requires a new TLS handshake and MQTT connection establishment.

.. _prereq_connect_to_azure_iot_hub:

Setup and configuration
***********************

To connect a device to Azure IoT Hub, complete the following steps:

1. :ref:`azure_set_up`
#. :ref:`azure_authenticate`
#. :ref:`azure_create_iot_hub`
#. :ref:`azure_generating_certificates`
#. :ref:`azure_create_device`
#. :ref:`azure_iot_hub_flash_certs`

.. rst-class:: numbered-step

.. _azure_set_up:

Setting up Azure IoT Hub
========================

If you do not have an Azure account, you need to create one.

To get started with testing the Azure IoT Hub, make sure that the following prerequisites are met:

* Install the `Azure CLI`_.
* To use the ``nrfcredstore`` tool, the dependencies in the :file:`nrf/scripts/requirements-extra.txt` file must be installed.

.. rst-class:: numbered-step

.. _azure_authenticate:

Authenticate Azure CLI
======================

Authenticate the Azure CLI tool to use your Azure account in the default browser with the following command:

.. code-block:: console

   az login

For other authentication options, see the `Sign in with Azure CLI`_ documentation.

.. rst-class:: numbered-step

.. _azure_create_iot_hub:

Create an IoT Hub
=================

1. When creating an IoT Hub, you must create it in a resource group.
   You can create a resource group through Azure's CLI using the following command:

   .. code-block:: console

      az group create --name <resource_name> --location westus

   If you want to use another region than ``westus``, you can acquire a list of the available locations by running the following command:

   .. code-block:: console

      az account list-locations -o table


#. To create an IoT Hub, use the following command, select the resource group you created, and create a unique name for your IoT Hub:

   .. code-block:: console

      az iot hub create --resource-group <resource_name> --name <hub_name> --sku F1 --partition-count 2

   Using ``F1`` as an argument creates a free IoT Hub, which allows you to have only one instance.
   Hence, delete your existing free IoT Hub or change the SKU to ``S1``.

For information on how to set up creating an Azure IoT Hub instance using the Azure portal, see `Creating an Azure IoT Hub instance using the Azure portal`_.

.. rst-class:: numbered-step

.. _azure_generating_and_provisioning_certificates:
.. _azure_generating_certificates:

Generating certificates
=======================

The connection to the Azure IoT Hub with MQTT is secured using TLS.
To create the device certificate, you need a Certificate Authority (CA) certificate and a private key that is used to sign all of your client certificates.
The CA certificate is uploaded to Azure IoT Hub, so Azure can verify that the device certificate is signed by your CA.
If you do not have a CA certificate, you can purchase one or make a self-signed test CA certificate for testing purposes.

To help generate test CA certificates and handle the device keys and certificates, you can use the :file:`nrf/scripts/cert_tool.py` Python script.
Either call the script relative from the current working directory or add it to the path.

.. note::
   The :file:`cert_tool.py` Python script has default values for all actions for the input and output file names.
   See the available arguments by running the ``--help`` argument to the script.

Generate test CA certificates
-----------------------------

* To generate the root CA certificate, use the following command:

  .. code-block:: console

     cert_tool.py root_ca

  This command generates a self-signed root CA certificate and private key and saves them to the files :file:`ca/root-ca-cert.pem` and :file:`ca/root-ca-key.pem`.

* To generate the subordinate CA certificate, use the following command:

  .. code-block:: console

     cert_tool.py sub_ca

  This command generates a subordinate CA certificate (signed by the root CA) and private key and saves them to the files :file:`ca/sub-ca-cert.pem` and :file:`ca/sub-ca-key.pem`.

Upload and verify the root CA certificate
-----------------------------------------

To perform proof of possession of the root CA key, you can verify the root CA certificate using the following set of commands:

* To upload root CA certificate::

   az iot hub certificate create --hub-name <hub_name> --name <cert_name> --path ca/root-ca-cert.pem

* To ask Azure for a verification code (need two output values)::

   az iot hub certificate generate-verification-code --hub-name <hub_name> --name <cert_name> --etag "<etag_from_prev_command>"

  Note down the verification code and etag for later use.

* To generate a new private key::

   cert_tool.py client_key

* To Create a CSR with the verification code as common name::

   cert_tool.py csr --common-name <verification_code>

* To Sign the CSR with the root CA::

   cert_tool.py sign_root

* To Upload the verification certificate::

   az iot hub certificate verify --hub-name <hub_name> --name <cert_name> --etag "<etag_from_generate_verification_code>" --path certs/client-cert.pem

.. _azure_device_provisioning:

Setup Device Provisioning Service (DPS)
---------------------------------------

If you are using DPS to provision devices to your IoT hub, you need to set up an Azure IoT Hub Device Provisioning Service (DPS) instance.

There are many ways to configure DPS.
Attestation using the subordinate CA certificate is one of them.
For other DPS configurations, see the `Azure IoT Hub Device Provisioning Service (DPS)`_ documentation.

Use the following commands to set up DPS using a subordinate CA certificate for attestation:

* To create the DPS instance::

   az iot dps create --name <dps_name> --resource-group <resource_name>

* To link the IoT Hub to the DPS instance::

   az iot dps linked-hub create --dps-name <dps_name> --hub-name <hub_name> --resource-group <resource_name>

* To Create an enrollment group::

   az iot dps enrollment-group create --dps-name <dps_name> --resource-group <resource_name> --enrollment-id <enrollment_name> --certificate-path ca/sub-ca-cert.pem --provisioning-status enabled --iot-hubs <iothub_url> --allocation-policy static

.. _azure_generate_certificates:

Generate and provision device certificates
------------------------------------------

The following are the ways to generate and register device certificates:

* The device key and certificate are generated using the :file:`cert_tool.py` script and provisioned to the device.
* The device generates a key and a Certificate Signing Request (CSR).
  This method is more secure because the private key never leaves the device.

.. tabs::

   .. tab:: nRF91: Modem generated private key

      When the private key is generated inside the modem, it is never exposed to the outside world.
      This is more secure than generating a private key using a script.
      By default, the Common Name (CN) of the certificate is set to the device UUID, and the CN is read out by Azure to identify the device during the TLS handshake.
      This allows using the exact same firmware on many devices without hard-coding the device ID into the firmware because the device UUID can be read out at runtime.
      See the :ref:`azure_iot_hub` sample for more information on how to read out the device UUID in the application.

      .. note::
         Generating a key pair on device requires an nRF91 Series device.
         If you are using an nRF9160 DK, modem version v1.3.x or later is required.

      .. important::
         Program the :ref:`at_client_sample` sample to your device before following this guide.

      Complete the following steps to generate a key pair and CSR on the modem, which is then signed using your CA and uploaded to Azure:

      1. Obtain a list of installed keys using the following command:

         .. code-block:: console

            nrfcredstore <serial port> list

         where ``<serial port>`` is the serial port of your device.

      #. Select a security tag that is not yet in use.
         This security tag must match the value set in the :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG` Kconfig option.

      #. Generate a key pair and obtain a CSR using the following command:

         .. code-block:: console

            nrfcredstore <serial port> generate <sec tag> certs/client-csr.der

         |serial_port_sec_tag|

      #. Convert the CSR from DER format to PEM format using the following command:

         .. code-block:: console

            openssl req -inform DER -in certs/client-csr.der -outform PEM -out certs/client-csr.pem

      #. Sign the CSR using the subordinate CA certificate using the following command:

         .. code-block:: console

            cert_tool.py sign

         .. note::
            This process might vary depending on the CA you are using.
            See the documentation for your CA for more information on how to sign a CSR.

      #. Take note of the CN, as it will be required later.

         In case you got the certificate from a CA, you can extract the CN using the following command:

         .. code-block:: console

            openssl x509 -in certs/client-cert.pem -noout -subject

      #. Provision the certificate to the modem using the following command:

         .. code-block:: console

            nrfcredstore <serial port> write <sec tag> CLIENT_CERT certs/client-cert.pem

         |serial_port_sec_tag|

   .. tab:: nRF91: Script generated private key

      .. caution::
         When generating the private key on your computer, make sure to keep it secure and not share it with anyone.
         If the private key is compromised, the security of the device is compromised.

      .. important::
         Program the :ref:`at_client_sample` sample to your device before following this guide.

         To obtain a key and certificate generated by the :file:`cert_tool.py` script and to provision them to the modem, complete the following steps:

      1. Generate the key and certificate using the following commands:

         .. code-block:: console

            cert_tool.py client_key
            cert_tool.py csr --common-name <device_id>
            cert_tool.py sign

         This command generates an elliptic curve private key and saves it to the :file:`certs/private-key.pem` file.
         The certificate is saved to the :file:`certs/client-cert.pem` file.

      #. Obtain a list of installed keys using the following command:

         .. code-block:: console

            nrfcredstore <serial port> list

         where ``<serial port>`` corresponds to the serial port of your device.

      #. Select a security tag that is not yet in use.
         This security tag must match the value set in :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG` Kconfig option.

      #. Provision the client certificate using the following command:

         .. code-block:: console

            nrfcredstore <serial port> write <sec tag> CLIENT_CERT certs/client-cert.pem

         |serial_port_sec_tag|

      #. Provision the client key using the following command:

         .. code-block:: console

            nrfcredstore <serial port> write <sec tag> CLIENT_KEY certs/private-key.pem

         |serial_port_sec_tag|

   .. tab:: nRF70: Script generated private key

      .. caution::
         When generating the private key on your computer, make sure to keep it secure and not share it with anyone.
         If the private key is compromised, the security of the device is compromised.

      To obtain a key and certificate generated using the :file:`cert_tool.py` script, complete the following steps:

      1. Generate the key and certificate using the following commands:

         .. code-block:: console

            cert_tool.py client_key
            cert_tool.py csr --common-name <device_id>
            cert_tool.py sign

      #. Provision the certificates and private key at runtime to the Mbed TLS stack.
         This is achieved by placing the PEM files into a :file:`certs/` subdirectory and ensuring the :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES` Kconfig option is enabled.
         For more information, refer to the :ref:`azure_iot_hub` sample as well as the :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FILE` Kconfig option.

.. rst-class:: numbered-step

.. _azure_create_device:

Register a device in Azure IoT Hub
==================================

.. tabs::

   .. tab:: Enroll using DPS

      .. code-block:: console

         az iot dps enrollment create --enrollment-id <cert_common_name> --device-id <cert_common_name> --provisioning-status enabled --resource-group <resource_group> --iot-hubs <iothub_name> --attestation-type x509 --certificate-path certs/client-cert.pem --dps-name <dps_name> --allocation-policy static

   .. tab:: Register each device by device ID

      .. important::
         The device ID must match the CN of the certificate.

      To register a new device in your IoT hub, use the following command:

      .. code-block:: console

         az iot hub device-identity create -n <iothub_name> -d <device_id> --am x509_ca


.. rst-class:: numbered-step

.. _azure_iot_hub_flash_certs:

Provision root CA certificates
==============================

The Azure IoT Hub library requires provisioning of the following certificates and a private key for a successful TLS connection:

1. `DigiCert Global Root G2`_ - The root CA certificate used to verify the server's certificate chain while connecting.
#. `Baltimore CyberTrust Root Certificate`_ - Azure's legacy root CA certificate needed to verify the Azure server's that have not migrated to `DigiCert Global Root G2`_ yet.
#. Device certificate - Generated by the procedures described in :ref:`azure_generate_certificates`, used by Azure IoT Hub to authenticate the device.
#. Private key of the device.

.. important::
   Azure has started the process of migrating their DPS server certificates from `Baltimore CyberTrust Root Certificate`_ to `DigiCert Global Root G2`_.
   Azure IoT Hub servers have finished this transition, and only DigiCert Global Root G2 is used now for those connections.
   Azure advises to have both Baltimore CyberTrust Root and DigiCert Global Root G2 certificates for all devices to avoid disruption of service during the transition.
   Refer to `Azure IoT TLS: Critical changes`_ for updated information and timeline.
   Due to this, it is recommended to provision the Baltimore CyberTrust Root Certificate to a secondary security tag set by the :kconfig:option:`CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG` option.
   This ensures that the device can also connect after the transition.

To provision the certificates, use any of the following methods, depending on the DK you are using.

.. tabs::

   .. tab:: nRF91: ``nrfcredstore``

      .. important::
         Program the :ref:`at_client_sample` sample to your device before following this guide and make sure you have ``nrfcredstore`` installed.

      1. Obtain a list of installed keys using the following command:

         .. code-block:: console

            nrfcredstore <serial port> list

         where ``<serial port>`` is the serial port of your device.

      #. Provison the server root CA certificates, which you downloaded previously, by running the following commands:

         .. code-block:: console

            nrfcredstore <serial port> write <sec tag> ROOT_CA_CERT DigiCertGlobalRootG2.crt.pem

         .. code-block:: console

            nrfcredstore <serial port> write <secondary sec tag> ROOT_CA_CERT BaltimoreCyberTrustRoot.crt.pem


   .. tab:: nRF91: nRF connect for Desktop

      .. include:: /includes/cert-flashing.txt

   .. tab:: nRF70: runtime provisioning

         Provision the certificates and private key at runtime to the Mbed TLS stack.
         This is achieved by placing the PEM files into a :file:`certs/` subdirectory and ensuring the :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES` Kconfig option is enabled.
         Save the :file:`DigiCertGlobalRootG2.crt.pem` file as :file:`certs/ca-cert.pem`, and the :file:`BaltimoreCyberTrustRoot.crt.pem` file as :file:`certs/ca-cert-2.pem`.
         For more information, refer to the :ref:`azure_iot_hub` sample as well as the :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FILE` Kconfig option.

         The CA will be provisioned to the security tag set by the :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG` Kconfig option.

The chosen security tag while provisioning the certificates must be the same as the security tag configured by the :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG` option.

If more than one root server certificate is used, the second one can be provisioned to a different security tag and configured in the application using the :kconfig:option:`CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG` Kconfig option.
The modem checks both security tags if necessary when verifying the server's certificate.

Configuring the library
=======================

You can configure the library to connect to Azure IoT Hub with or without using DPS.

.. _dps_config:

.. tabs::

   .. tab:: Using DPS

      To connect to Azure IoT Hub using DPS, complete the following steps:

      1. Set the :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE` Kconfig option to the ``ID Scope`` for your DPS instance by running the following command:

         .. code-block:: console

            az iot dps show --name <dps_name> --query "properties.idScope"

         Alternatively, you can set the registration ID at runtime.

      #. Configure the registration ID:

         For testing on one device, you can manually configure the registration ID by setting the :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_REG_ID` Kconfig option.

         When running the same firmware on multiple devices, it is not practical to hard-code the registration ID.
         Instead enable the use of a unique hardware identifier, such as the device UUID, as the registration ID.
         The hardware identifier of the device needs to match the CN in the certificate on the device.

         .. tabs::

            .. tab:: nRF91: Device UUID

               .. code-block:: none

                  CONFIG_MODEM_JWT=y
                  CONFIG_HW_ID_LIBRARY_SOURCE_UUID=y

            .. tab:: nRF91: Device IMEI

               .. code-block:: none

                  CONFIG_HW_ID_LIBRARY_SOURCE_IMEI=y

            .. tab:: nRF70: Network MAC address

               .. code-block:: none

                  HW_ID_LIBRARY_SOURCE_NET_MAC=y


         .. note::
            When using hardware identifiers in the :ref:`azure_iot_hub` sample, set the :kconfig:option:`CONFIG_AZURE_IOT_HUB_SAMPLE_DEVICE_ID_USE_HW_ID` Kconfig option to ``y``.

         You can also set the registration ID at runtime.

      #. Set the :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG` Kconfig option to the security tag used while :ref:`provisioning root CA certificates <azure_iot_hub_flash_certs>`.
      #. Set the :kconfig:option:`CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG` Kconfig option to the security tag of the extra root CA certificate until all Azure services have migrated to DigiCert Global Root G2.

   .. tab:: Without DPS

      To connect to Azure IoT Hub without using DPS, complete the following minimum required configuration:

      1. To retrieve your IoT Hub hostname, run the following command:

         .. code-block:: console

            az iot hub show --name <hub_name> --query "properties.hostName"

      #. Configure the :kconfig:option:`CONFIG_AZURE_IOT_HUB_HOSTNAME` Kconfig option to the returned address.

         You can also set the host name at runtime.
      #. Set the Kconfig option :kconfig:option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID` to the device ID.

         The device ID must match with the one used while creating the certificates.
         You can also set the device ID at runtime by populating the ``device_id`` member of the :c:struct:`azure_iot_hub_config` structure passed to the :c:func:`azure_iot_hub_connect` function when connecting.
         If the ``device_id.size`` buffer size is zero, the compile-time option :kconfig:option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID` is used.
      #. Make sure that the device is already registered with your Azure IoT Hub, or follow the instructions in :ref:`azure_create_device`.
      #. Set the :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG` Kconfig option to the security tag used in :ref:`azure_iot_hub_flash_certs`.
      #. Set the :kconfig:option:`CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG` Kconfig option to the security tag of the extra root CA certificate until all Azure services has migrated to DigiCert Global Root G2.

Application integration
***********************

This section describes how to initialize the library, use the DPS service, and connect to Azure IoT Hub.

Initializing the library
========================

To initialize the library, call the :c:func:`azure_iot_hub_init` function.
The initialization must be successful to make the other APIs in the library available for the application.
An event handler is passed as the only argument to the :c:func:`azure_iot_hub_init` function.
The library calls this function with data associated to the application, such as incoming data and other events.
For an exhaustive list of event types and associated data, see :c:enum:`azure_iot_hub_evt_type`.

Using the Device Provisioning Service
=====================================

You can use the Azure IoT Hub Device Provisioning Service to provision the device to an IoT Hub.
When the registration process has completed successfully, the device receives its assigned hostname and device ID to use when connecting to Azure IoT Hub.
The assigned host name and device ID are stored to the non-volatile memory on the device and are available also after a reset and power outage.

This code example shows how to configure and use DPS:

.. code-block:: c

   static void dps_handler(enum azure_iot_hub_dps_reg_status state)
   {
      switch (state) {
      case AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED:
         LOG_INF("AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED");
         break;
      case AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING:
         LOG_INF("AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING");
         break;
      case AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED:
         LOG_INF("AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED");

         /* Act on assignment */
         k_sem_give(&dps_assigned_sem);
         break;
      case AZURE_IOT_HUB_DPS_REG_STATUS_FAILED:
         LOG_INF("ZURE_IOT_HUB_DPS_REG_STATUS_FAILED");

         /* Act on registration failure */
         k_sem_give(&dps_registration_failed_sem);
         break;
      default:
         LOG_WRN("Unhandled DPS registration status: %d", state);
         break;
      }
   }

   ...

   int err;
   struct azure_iot_hub_buf assigned_hostname;
   struct azure_iot_hub_buf assigned_device_id;
	struct azure_iot_hub_dps_config dps_cfg = {
		.handler = dps_handler,

      /* Can be left out to use CONFIG_AZURE_IOT_HUB_DPS_REG_ID instead. */
		.reg_id = {
			.ptr = device_id_buf,
			.size = device_id_len,
		},

      /* Can be left out to use CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE instead. */
      .id_scope = {
			.ptr = id_scope_buf,
			.size = id_scope_len,
		},
	};

	err = azure_iot_hub_dps_init(&dps_cfg);
   /* Error handling */

   err = azure_iot_hub_dps_start();
	if (err == 0) {
		LOG_INF("The DPS process has started");

      /* Wait for the registration process to complete. */
      err = k_sem_take(&dps_done_sem, K_SECONDS(SOME_TIMEOUT));
      /* Error handling */
	} else if (err == -EALREADY) {
		LOG_INF("Already assigned to an IoT hub, skipping DPS");
	} else {
      /* Error handling */
	}
	err = azure_iot_hub_dps_hostname_get(assigned_hostname);
   /* Error handling */

	err = azure_iot_hub_dps_device_id_get(assigned_device_id);
   /* Error handling */

   /* Use the hostname and device ID to connect to IoT Hub. */

After the device has been successfully registered, the application can proceed to connect to the assigned IoT Hub using the obtained device ID.

When a device has been assigned to an IoT Hub and the information is stored to the non-volatile memory, the DPS APIs always return the stored information and do not trigger a new registration.
To delete the stored assignment information, call the :c:func:`azure_iot_hub_dps_reset` function.
Alternatively, you can call the functions :c:func:`azure_iot_hub_dps_hostname_delete` or :c:func:`azure_iot_hub_dps_device_id_delete` to delete specific information.
After calling the :c:func:`azure_iot_hub_dps_reset` function, the library must be initialized again.
After the initialization, a new registration with the DPS can be started by calling the :c:func:`azure_iot_hub_dps_start` function.

The DPS APIs are documented in the :ref:`azure_iot_hub_dps_api` section.

Connecting to Azure IoT Hub
===========================

After the initialization, calling the :c:func:`azure_iot_hub_connect` function connects the device to the configured IoT hub or DPS instance, depending on the configuration.
The initial TLS handshake takes a few seconds to complete, depending on the network conditions and the TLS cipher suite used.
During the TLS handshake, the :c:func:`azure_iot_hub_connect` function blocks.
Consider this when deciding the context from which the API is called.
Optionally, DPS registration can be run automatically as part of the call to the :c:func:`azure_iot_hub_connect` function.

.. note::
   The :c:func:`azure_iot_hub_connect` function blocks when DPS registration is pending.
   Running DPS as part of the :c:func:`azure_iot_hub_connect` function also limits the DPS configuration options as follows:

   * The device ID is used as registration ID when registering with the DPS server.
   * The ID scope is set in the :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE` option.

   Use the DPS APIs directly if you need more control over the DPS registration process.

When using the :c:func:`azure_iot_hub_connect` function, you can choose to provide the host name to the IoT Hub and device ID at runtime, or let the library use Kconfig options.

Here is an example for setting the host name and device ID at runtime:

.. code-block:: c

   struct azure_iot_hub_config cfg = {
      .hostname = {
         .ptr = hostname_buffer,
         .size = hostname_length,
      },
      .device_id = {
         .ptr = device_id_buffer,
         .size = device_id_length,
      },
      .use_dps = false,
   };

   err = azure_iot_hub_connect(&cfg);
   /* Error handling */

You can pass ``NULL`` or a zeroed-out configuration to the :c:func:`azure_iot_hub_connect` function.
The library uses the values for host name and device ID from the Kconfig options :kconfig:option:`CONFIG_AZURE_IOT_HUB_HOSTNAME` and :kconfig:option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID`, respectively.

This code example uses a Kconfig value for the device ID (and by extension DPS registration ID) and runs DPS to acquire the assigned IoT Hub host name and assigned device ID.

.. code-block:: c

   struct azure_iot_hub_config cfg = {
      .use_dps = true,
   };

   err = azure_iot_hub_connect(&cfg);
   /* Error handling */

After a successful connection, the library automatically subscribes to the following standard Azure IoT Hub MQTT topics (See `Azure IoT Hub MQTT protocol support`_ for details):

* ``devices/<device ID>/messages/devicebound/#`` (cloud-to-device messages)
* ``$iothub/twin/PATCH/properties/desired/#`` (desired properties update notifications)
* ``$iothub/twin/res/#`` (operation responses)
* ``$iothub/methods/POST/#`` (direct method requests)

Currently, the library does not support persistent MQTT sessions.
Hence subscriptions are requested for each connection to the IoT hub.

For more information about the available APIs, see the :ref:`azure_iot_hub_api` section.


Configuration
*************

To use the Azure IoT Hub library, you must enable the :kconfig:option:`CONFIG_AZURE_IOT_HUB` Kconfig option.

You can configure the following options when using this library:

* :kconfig:option:`CONFIG_AZURE_IOT_HUB_HOSTNAME` - Sets the Azure IoT Hub host name. Note that the host name can also be provided at runtime.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID` - Configures the device ID. The device ID can also be set at runtime.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_USER_NAME_BUF_SIZE` - Sets the user name buffer size. You can adjust the  buffer size to reduce stack usage, if you know the approximate size of your device ID.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_AUTO_DEVICE_TWIN_REQUEST` - Automatically requests the device twin upon connection to an IoT Hub.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_TOPIC_MAX_LEN` - Sets the maximum topic length. The topic buffers are allocated on the stack. You may have to adjust this option to match with your device ID length.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_MSG_PROPERTY_RECV_MAX_COUNT` - Sets the maximum number of message properties that can be parsed from an incoming message's topic.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_MSG_PROPERTY_BUFFER_SIZE` - Sets the size of the internal message property buffer used when sending messages with message properties, allocated on the stack. You can adjust this to fit your needs.

MQTT helper library specific options:

* :kconfig:option:`CONFIG_MQTT_HELPER_SEND_TIMEOUT` - Enables timeout when sending data to an IoT Hub.
* :kconfig:option:`CONFIG_MQTT_HELPER_SEND_TIMEOUT_SEC` - Sets the send timeout value (in seconds) to use when sending data.
* :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG` - Sets the security tag where the Azure IoT Hub certificates are stored.
* :kconfig:option:`CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG` - Sets the secondary security tag that can be used for a second CA root certificate.
* :kconfig:option:`CONFIG_MQTT_HELPER_PORT` - Sets the TCP port number to connect to.
* :kconfig:option:`CONFIG_MQTT_HELPER_RX_TX_BUFFER_SIZE` - Sets the size of the MQTT RX and TX buffer that limits the message size, excluding the payload size.
* :kconfig:option:`CONFIG_MQTT_HELPER_PAYLOAD_BUFFER_LEN` - Sets the MQTT payload buffer size.
* :kconfig:option:`CONFIG_MQTT_HELPER_STACK_SIZE` - Sets the stack size for the internal thread in the library.
* :kconfig:option:`CONFIG_MQTT_HELPER_NATIVE_TLS` - Configures the socket to be native for TLS instead of offloading TLS operations to the modem.

DPS-specific configuration:

* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS` - Enables Azure IoT Hub DPS.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME` - Host name of the DPS server.
   Do not change this unless you have configured DPS to use a different host name.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_REG_ID` - Registration ID to use in the registration request to DPS.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME_MAX_LEN` - Maximum length of the assigned host name received from DPS.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_DEVICE_ID_MAX_LEN` - Maximum length of the assigned device ID received from DPS.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_TOPIC_BUFFER_SIZE` - Size of the internal topic buffers in the DPS library.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_USER_NAME_BUFFER_SIZE` - User name buffer size.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE` - Sets the Azure IoT Hub DPS ID scope that is used while provisioning the device.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_OPERATION_ID_BUFFER_SIZE` - Size of the operation ID buffer. The operation ID is received from the IoT Hub during registration.

API documentation
*****************

.. _azure_iot_hub_api:

Azure IoT Hub API
=================

| Header file: :file:`include/net/azure_iot_hub.h`
| Source files: :file:`subsys/net/lib/azure_iot_hub/src/azure_iot_hub.c`

.. doxygengroup:: azure_iot_hub
   :project: nrf
   :members:

.. _azure_iot_hub_dps_api:

Azure IoT Hub DPS API
=====================

| Header file: :file:`include/net/azure_iot_hub_dps.h`
| Source files: :file:`subsys/net/lib/azure_iot_hub/src/azure_iot_hub_dps.c`

.. doxygengroup:: azure_iot_hub_dps
   :project: nrf
   :members:
