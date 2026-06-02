.. _mqtt_sample_provisioning:

Provisioning
############

.. contents::
   :local:
   :depth: 2

.. note::

   This section is relevant only if you build the sample with TLS enabled.

The server CA for the default MQTT broker (`mqtt.nordicsemi.academy`_) is provisioned to the network stack at runtime before establishing a connection to the server.
The server CA (:file:`ca-cert.pem`) is located in the :file:`src/modules/transport/credentials/` folder.
Ensure that if the server is changed, the CA needs to be updated as well.

To retrieve the server CA for a different MQTT broker, use the following command:

.. code-block:: console

   openssl s_client -connect <hostname>:<port> -showcerts -servername <hostname>

It is recommended to use the root (topmost) certificate of the returned certificate chain, as it is more stable.
Intermediate certificates (located further down the chain) might change over time.

.. important::

   Provisioning of credentials at runtime is only meant for testing purposes.
   This must be avoided in a production scenario, especially if using client authenticated TLS where the private key will be exposed in flash.

To turn off runtime credential provisioning, disable either of the following Kconfig options:

* :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES` - For native simulator and nRF70 Series builds.
* :kconfig:option:`CONFIG_MODEM_KEY_MGMT` - For nRF91 Series builds.

The CA is provisioned to the security tag set by the :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG` Kconfig option.

By default, the established TLS connection to the default MQTT broker (`mqtt.nordicsemi.academy`_) does not require a client authentication, which removes the need to provision a client certificate and private key to the network stack.
If the client certificate and private key has been generated for a server connection, the credentials must be provisioned the same way as the server CA.
This occurs automatically when including the corresponding files :file:`src/modules/transport/credentials/client-cert.pem` and :file:`src/modules/transport/credentials/private-key.pem`.

.. include:: /includes/cert-flashing.txt
