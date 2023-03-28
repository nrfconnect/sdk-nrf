.. _anjay_lwm2m_client:

Anjay: LwM2M Client
###################

.. contents::
   :local:
   :depth: 2

The Anjay LwM2m client sample demonstrates usage of the :term:`Lightweight Machine to Machine (LwM2M)` protocol to
connect Thingy:91, nRF52840DK or an nRF9160DK to an LwM2M server through LTE. This samples uses AVSystem's `Anjay-zephyr`_ library.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

There's also an alternative configuration for nRF9160DK, revisions 0.14.0 and up, which utilizes external flash chip to
perform firmware updates.

.. include:: /includes/tfm.txt

Additionally, the sample requires an activated SIM card, and an LwM2M server such as `Coiote Device Management`_
server. To know more about the AVSystem integration with nRF Connect SDK, see :ref:`ug_avsystem`.

Overview
********

LwM2M is na application protocol based on CoAP. It is designed to expose various resources for reading, writing, and
executing through an LwM2M server in a very lightweight environment. The client sends data such as sensor readings and
GNSS position to the LwM2M server. It can also receive activation commands from the server.

The sample implements the following LwM2M objects:

.. list-table:: LwM2M objects
   :header-rows: 1

   *  - LwM2M objects
      - Object ID
      - Thingy:91
      - nRF9160DK
      - nRF52840DK
   *  - Security
      - \\0
      - Yes
      - Yes
      - Yes
   *  - Server
      - \\1
      - Yes
      - Yes
      - Yes
   *  - Device
      - \\3
      - Yes
      - Yes
      - Yes
   *  - Connectivity Monitoring
      - \\4
      - Yes
      - Yes
      - No
   *  - Firmware Update
      - \\5
      - Yes
      - Yes
      - No
   *  - Location
      - \\6
      - Yes (configurable in Kconfig)
      - Yes (configurable in Kconfig)
      - No
   *  - Temperature
      - \\3303
      - Yes
      - No
      - No
   *  - Humidity
      - \\3304
      - Yes
      - No
      - No
   *  - Accelerometer
      - \\3313
      - Yes
      - No
      - No
   *  - Barometer
      - \\3315
      - Yes
      - No
      - No
   *  - Buzzer
      - \\3338
      - Yes
      - No
      - No
   *  - On/Off Switch
      - \\3342
      - No
      - Yes
      - No
   *  - Push Button
      - \\3347
      - Yes
      - Yes
      - Yes
   *  - LED color light
      - \\3420
      - Yes
      - No
      - No
   *  - ECID-Signal Measurement Information
      - \\10256
      - Yes
      - Yes
      - No
   *  - Location Assistance
      - \\50001
      - Yes (experimental)
      - Yes (experimental)
      - No


Compilation
***********

Basic configuration
===================

You can compile basic configuration of the client using:

   .. code-block::

      west build -b <board_name>

where ``<board_name>`` should be replaced with ``thingy91_nrf9160_ns``, ``nrf9160dk_nrf9160_ns`` or
``nrf52840dk_nrf52840`` depending on your board.

Compiling for external flash usage
==================================

For nRF9160DK hardware revisions 0.14.0 and up, an alternate configuration that puts the MCUboot secondary partition on
the external flash instead of dividing the internal flash space is available. To compile in this configuration, use the
following command:

   .. code-block::

      west build -b nrf9160dk_nrf9160_ns@0.14.0 -- -DCONF_FILE=prj_extflash.conf

Compiling with software-based cryptography
==========================================

Normally on Nordic boards, security is provided using the (D)TLS sockets implemented in modem firmware and provided by
nrfxlib. However, on nRF9160DK revisions 0.14.0 and up, it is possible to switch to software-based implementation based
on Mbed TLS instead. This is not recommended due to lowered security and performance, but may be desirable if you
require some specific (D)TLS features (e.g. ciphersuites) that are not supported by the modem.

To compile in this configuration, use the following command:

   .. code-block::

      west build -b nrf9160dk_nrf9160_ns@0.14.0 -- -DCONF_FILE=prj_extflash.conf -DOVERLAY_CONFIG=overlay_nrf_mbedtls.conf

Configuration options
*********************

Anjay Kconfig options
=====================

* :kconfig:option:`CONFIG_ANJAY_CLIENT_BUILD_MCUBOOT_AUTOMATICALLY` - Build MCUboot as part of the main build process
* :kconfig:option:`CONFIG_ANJAY_COMPAT_MBEDTLS` - Use the Mbed TLS backend
* :kconfig:option:`CONFIG_ANJAY_COMPAT_ZEPHYR_TLS` - Use Zephyr TLS sockets
* :kconfig:option:`CONFIG_ANJAY_COMPAT_ZEPHYR_TLS_SESSION_CACHE` - Enable Zephyr TLS session cache
* :kconfig:option:`CONFIG_ANJAY_COMPAT_ZEPHYR_TLS_EPHEMERAL_SEC_TAG_BASE` - Lowest security tag number to use for Anjay's ephemeral credentials
* :kconfig:option:`CONFIG_ANJAY_COMPAT_ZEPHYR_TLS_EPHEMERAL_SEC_TAG_COUNT` - Numer of security tags to reserve for Anjay's ephemeral credentials
* :kconfig:option:`CONFIG_ANJAY_COMPAT_ZEPHYR_TLS_ASSUME_RESUMPTION_SUCCESS` - Assume that (D)TLS session resumption always succeeds
* :kconfig:option:`CONFIG_ANJAY_COMPAT_NET` - Enable Net compatibility implementation
* :kconfig:option:`CONFIG_ANJAY_COMPAT_TIME` - Enable Time compatibility implementation
* :kconfig:option:`CONFIG_ANJAY_WITH_LOGS` - Enable logging in Anjay, avs_commons and avs_coap
* :kconfig:option:`CONFIG_ANJAY_WITH_TRACE_LOGS` - Enable TRACE-level logs in Anjay, avs_commons and avs_coap
* :kconfig:option:`CONFIG_ANJAY_WITH_MICRO_LOGS` - Enable the "micro logs" feature
* :kconfig:option:`CONFIG_ANJAY_WITH_ACCESS_CONTROL` - Enable core support for Access Control mechanisms
* :kconfig:option:`CONFIG_ANJAY_WITH_DOWNLOADER` - Enable support for the anjay_download() API
* :kconfig:option:`CONFIG_ANJAY_WITH_COAP_DOWNLOAD` - Enable support for CoAP(S) downloads
* :kconfig:option:`CONFIG_ANJAY_WITH_HTTP_DOWNLOAD` - Enable support for HTTP(S) downloads
* :kconfig:option:`CONFIG_ANJAY_WITH_BOOTSTRAP` - Enable support for the LwM2M Bootstrap Interface
* :kconfig:option:`CONFIG_ANJAY_WITH_DISCOVER` - Enable support for the LwM2M Discover operation
* :kconfig:option:`CONFIG_ANJAY_WITH_OBSERVE` - Enable support for the LwM2M Information Reporting interface
* :kconfig:option:`CONFIG_ANJAY_WITH_NET_STATS` - Enable support for measuring amount of LwM2M traffic
* :kconfig:option:`CONFIG_ANJAY_WITH_COMMUNICATION_TIMESTAMP_API` - Enable support for communication timestamp API
* :kconfig:option:`CONFIG_ANJAY_WITH_OBSERVATION_STATUS` - Enable support for the anjay_resource_observation_status() API
* :kconfig:option:`CONFIG_ANJAY_MAX_OBSERVATION_SERVERS_REPORTED_NUMBER` - Maximum number of listed servers that observe a given Resource
* :kconfig:option:`CONFIG_ANJAY_WITH_THREAD_SAFETY` - Enable guarding of all accesses to anjay_t with a mutex
* :kconfig:option:`CONFIG_ANJAY_WITH_EVENT_LOOP` - Enable default implementation of the event loop
* :kconfig:option:`CONFIG_ANJAY_WITH_LWM2M11` - Enable support for features new to LwM2M protocol version 1.1
* :kconfig:option:`CONFIG_ANJAY_WITH_SEND` - Enable support for the LwM2M Send operation
* :kconfig:option:`CONFIG_ANJAY_WITH_LEGACY_CONTENT_FORMAT_SUPPORT` - Enable support for legacy CoAP Content-Format values
* :kconfig:option:`CONFIG_ANJAY_WITH_LWM2M_JSON` - Enable support for JSON format as specified in LwM2M TS 1.0
* :kconfig:option:`CONFIG_ANJAY_WITHOUT_TLV` - Disable support for TLV format as specified in LwM2M TS 1.0
* :kconfig:option:`CONFIG_ANJAY_WITHOUT_PLAINTEXT` - Disable support for Plain Text format as specified in LwM2M TS 1.0. and 1.1
* :kconfig:option:`CONFIG_ANJAY_WITHOUT_DEREGISTER` - Disable use of the Deregister message
* :kconfig:option:`CONFIG_ANJAY_WITHOUT_IP_STICKINESS` - Disable support for "IP stickiness"
* :kconfig:option:`CONFIG_ANJAY_WITH_SENML_JSON` - Enable support for SenML JSON format, as specified in LwM2M TS 1.1
* :kconfig:option:`CONFIG_ANJAY_WITH_CBOR` - Enable support for CBOR and SenML CBOR formats, as specified in LwM2M TS 1.1
* :kconfig:option:`CONFIG_ANJAY_WITH_CON_ATTR` - Enable support for custom "con" attribute that controls Confirmable notifications
* :kconfig:option:`CONFIG_ANJAY_MAX_PK_OR_IDENTITY_SIZE` - Maximum size of the "Public Key or Identity"
* :kconfig:option:`CONFIG_ANJAY_MAX_SECRET_KEY_SIZE` - Maximum size of the "Secret Key"
* :kconfig:option:`CONFIG_ANJAY_MAX_DOUBLE_STRING_SIZE` - Maximum length supported for stringified floating-point values
* :kconfig:option:`CONFIG_ANJAY_MAX_URI_SEGMENT_SIZE` - Maximum length supported for a single Uri-Path or Location-Path segment
* :kconfig:option:`CONFIG_ANJAY_MAX_URI_QUERY_SEGMENT_SIZE` - Maximum length supported for a single Uri-Query segment
* :kconfig:option:`CONFIG_ANJAY_DTLS_SESSION_BUFFER_SIZE` - DTLS buffer size
* :kconfig:option:`CONFIG_ANJAY_WITH_ATTR_STORAGE` - Enable attr_storage module
* :kconfig:option:`CONFIG_ANJAY_WITH_MODULE_ACCESS_CONTROL` - Enable access control module
* :kconfig:option:`CONFIG_ANJAY_WITH_MODULE_SECURITY` - Enable security module
* :kconfig:option:`CONFIG_ANJAY_WITH_MODULE_SERVER` - Enable server module
* :kconfig:option:`CONFIG_ANJAY_WITH_MODULE_FW_UPDATE` - Enable fw_update module
* :kconfig:option:`CONFIG_ANJAY_WITH_MODULE_IPSO_OBJECTS` - Enable IPSO objects implementation
* :kconfig:option:`CONFIG_ANJAY_WITH_MODULE_FACTORY_PROVISIONING` - Enable factory provisioning module
* :kconfig:option:`CONFIG_ANJAY_WITH_NORDIC_LOCATION_SERVICES` - Enable Nordic location services

Anjay-zephyr Kconfig options
============================

* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_DEVICE_MANUFACTURER` - Device manufacturer
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_MODEL_NUMBER` - Model number
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_VERSION` - Client Version
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_THREAD_PRIORITY` - Priority of the Anjay thread
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_THREAD_STACK_SIZE` - Size of the Anjay thread stack
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_NTP_SERVER` - NTP Server
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_SHELL` - Enable Anjay shell commands
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_GPS_NRF` - Enable GPS on nRF9160-based devices
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_GPS_NRF_A_GPS` - Enable A-GPS using Nordic Location Services over LwM2M
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_LOCATION_SERVICES_MANUAL_CELL_BASED` - Enable manual requests for cell-based location
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_NRF_LC_INFO_CELL_POLL_RATE` - Current and neighbouring cells stats polling rate [seconds]
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_NETWORK_KEEPALIVE_RATE` - Rate of checking whether the network connection is still alive [seconds]
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_FOTA` - Enable the Firmware Update object
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_PERSISTENCE` - Enable persistence
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_FACTORY_PROVISIONING` - Use factory provisioning
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_FACTORY_PROVISIONING_INITIAL_FLASH` - Build the app in initial flashing mode
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_NRF_MODEM_PSK_QUERY` - Security tag for the credential stored in the modem
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_GPS`
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_LOCATION_OBJECT`
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_GPS_ALTITUDE`
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_GPS_RADIUS`
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_GPS_VELOCITY`
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_GPS_SPEED`
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_LOCATION_SERVICES`
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_LOCATION_SERVICES_ASSISTANCE`
* :kconfig:option:`CONFIG_ANJAY_ZEPHYR_NRF_LC_INFO`

Runtime certificate and private key configuration
=================================================

To build a project with runtime certificate and private key (not available for Thingy:91), use the following command:

   .. code-block::

      west build -b <board_name> -p -- -DCONFIG_ANJAY_ZEPHYR_RUNTIME_CERT_CONFIG=y

This feature works with nrf9160dk starting from revision v0.14.0. For this board use configuration which utilizes
external flash chip and software-based cryptography:

   .. code-block::

      west build -b nrf9160dk_nrf9160_ns@0.14.0 -p -- -DCONF_FILE=prj_extflash.conf -DOVERLAY_CONFIG="overlay_nrf_mbedtls.conf"

After that, certificate and private key based on SECP256R1 curve can be provided through shell interface in PEM format.
To generate them use following commands (to use certificate and private key with `Coiote Device Management`_ you must
specify a common name that is the same as the client endpoint name):

   .. code-block::

      openssl ecparam -name secp256r1 -out ecparam.der
      openssl req -new -x509 -nodes -newkey ec:ecparam.der -keyout demo-cert.key -out demo-cert.crt -days 3650
      openssl x509 -in demo-cert.crt -outform pem -out cert.pem
      openssl ec -in demo-cert.key -outform pem -out key.pem

Then provide the generated certificate and private key through the shell with the following commands respectively:

   .. code-block::

      anjay config set public_cert
      anjay config set private_key

Factory provisioning (experimental)
***********************************

.. note::

   All required files needed to use provisioning tool (including ``tools/`` directory) are **NOT** included in this sample application. For more information please visit `Anjay-zephyr-client Github repository`_.

Anjay LwM2M client supports experimental factory provisioning feature, thanks to which the user is able to
pre-provision credentials to the device using a specially tailored version of the application. This feature allows to
easily pre-provision large quantities of devices in a semi-automatic manner.

To use this feature, one can use a script ``tools/provisioning-tools/ptool.py``. It might be used in the similar manner as
the script of the same name described in the documentation: `Factory Provisioning Tool`_. There are a few new and
important command-line arguments:

   * ``--board`` (``-b``) - the board for which the images should be built,
   * ``--image_dir`` (``-i``) - directory for the cached Zephyr hex images,
   * ``--serial`` (``-s``) - serial number of the device to be used,
   * ``--baudrate`` (``-B``) - baudrate for the used serial port, when it is not provided the default value is 115200,
   * ``--conf_file`` (``-f``) - application configuration file(s) for final image build, by default, prj.conf is used.

If the image ``initial.hex`` exists in the given ``image_dir`` the initial provisioning image won't be built and the
same works for final image and ``final.hex``. When ``image_dir`` path is provided, but some images are missing, they
will be built in the given directory. If ``image_dir`` is not provided then the images will be built in
``$(pwd)/provisioning_builds``.

Before using the script make sure that in the shell in which you run it the ``west build`` command would work for a
selected board (please remember to update manifest file as described in the compiling guides above but with absolute
manifest.path) and that all of the configs passed to the script are valid - in particular, make sure that you changed
``<YOUR_DOMAIN>`` in ``tools/provisioning-tools/configs/lwm2m_server.json`` config file to your actual domain in EU
cloud Coiote installation (or fill the whole ``lwm2m_server.json`` and ``endpoint_cfg`` files with some different valid
server configuration).

Example script invocation provisioning some nRF9160DK board may look like:

   .. code-block::

      ../tools/provisioning-tool/ptool.py -b nrf9160dk_nrf9160_ns -s <SERIAL> -c ../tools/provisioning-tool/configs/endpoint_cfg -t <TOKEN> -S ../tools/provisioning-tool/configs/lwm2m_server.json

where ``<SERIAL>`` should be replaced by our board's serial number and ``<TOKEN>`` should be replaced by some valid
authentication token for the Coiote server provided in the ``lwm2m_server.json`` file.

The generation of token is explained in the Coiote documentation. In Coiote click on the question mark in the top right
corner, then Documentation -> User. The description can be found in `Rest API -> REST API authentication`_ section.

Using Certificate Mode with factory provisioning
================================================

If supported by the underlying (D)TLS backend (if using Mbed TLS, make sure that it is configured appropriately), the
application can authenticate with the server using certificate mode.

You will need to download the server certificate first. One possible way to do it is with ``openssl``:

   .. code-block::

      openssl s_client -showcerts -connect eu.iot.avsystem.cloud:5684 | openssl x509 -outform der -out eu-cloud-cert.der

.. note::

   Only servers that use self-signed certificates are reliably supported by default. You can change this behavior by setting the Certificate Usage resource in the endpoint configuration file. However, this might not be supported by all (D)TLS backends.

In particular, when ``CONFIG_ANJAY_COMPAT_ZEPHYR_TLS`` is enabled (which is the default for Nordic boards), the
Certificate Usage are only approximated by adding the server certificate to traditional PKIX trust store if Certificate
Usage is set to 2 or 3 (note that 3 is the default) and ignoring it otherwise.

You should then modify the cert_info.json file that's located in ``tools/provisioning-tool/configs`` for the desired
self-signed certificate configuration.

Once you have the server certificate, you can now provision the board. Example script invocation may look like:

   .. code-block::

      ../tools/provisioning-tool/ptool.py -b nrf9160dk_nrf9160_ns -s <SERIAL> -c ../tools/provisioning-tool/configs/endpoint_cfg_cert -p eu-cloud-cert.der -C ../tools/provisioning-tool/configs/cert_info.json

.. note::

   Coiote DM currently does not support registering devices together with uploading dynamically generated self-signed certificates using command-line tools.

   You will need to manually add the new device on Coiote DM via GUI and upload the certificate during the "Add device credentials" step.

Performing the FOTA (Firmware Over-The-Air)
*******************************************

To upgrade the firmware, upload the ``build/zephyr/app_update.bin`` image using standard means of LwM2M Firmware Update
object.

IoT Developer Zone
******************

To learn more about Anjay integrations and discover Nordic board tutorials, please go to AVsystem's `IoT Developer Zone`_. 

.. _Anjay-zephyr: https://github.com/AVSystem/Anjay-zephyr
.. _Coiote Device Management: https://www.avsystem.com/products/coiote-iot-device-management-platform/
.. _Anjay-zephyr-client Github repository: https://github.com/AVSystem/Anjay-zephyr-client
.. _Factory Provisioning Tool: https://avsystem.github.io/Anjay-doc/Tools/FactoryProvisioning.html
.. _Rest API -> REST API authentication: https://eu.iot.avsystem.cloud/doc/user/REST_API/REST_API_Authentication/
.. _IoT Developer Zone: https://iotdevzone.avsystem.com/docs/LwM2M_Client/Getting_started/
