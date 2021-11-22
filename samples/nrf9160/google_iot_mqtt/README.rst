.. _google-iot-mqtt-sample:

Google IOT MQTT Sample
######################

Overview
********

Based on the `Zephyr sample <https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/net/cloud/google_iot_mqtt>`_, and adapted for nRF9160, leveraging modem features.

This sample application demonstrates a "full stack" application.  This
currently is able to:

- Establish a TLS connection with the Google IOT Cloud servers
- Publish data to the Google IOT Cloud
- Send/Receive keep alive / pings from cloud server

The source code for this sample application can be found at:
:zephyr_file:`samples/net/cloud/google_iot_mqtt`.

Requirements
************
- Entropy source
- Google IOT Cloud account
- Google IOT Cloud credentials and required information
- Cellular connectivity

Building and Running
********************
This application has been built and tested on the Nordic nRF9160-DK.
ECDSA keys are required to authenticate to the Google IOT Cloud.
The application includes a key creation script.

Run ``bash create_keys.sh`` in the
``samples/net/cloud/google_iot_mqtt/src/private_info/`` directory.

Clone the `cred utility <https://github.com/inductivekickback/cred>`_ for programming credentials to the modem. 

Program the client private key:

.. code-block: bash
  python3 cred.py \
    --client_private_key ../google_iot_mqtt/src/private_info/my_device-ec_private.pem \
    --sec_tag 10

Download cloud-side certs (primary and backup):

- `primary <https://pki.goog/gtsltsr/gtsltsr.crt>`_

- `backup <https://pki.goog/gsr4/GSR4.crt>`_

NOTE: It is not necessary to change the certs to to "C-Style\\n" formatting. 

Program the cloud-side certs:

.. code-block: bash
  python3 cred.py \
    --CA_cert gtsltsr.pem \
    --sec_tag 202
  python3 cred.py \
    --CA_cert GSR4.pem \
    --sec_tag 203

Assign keys on Google Cloud IoT Core 
- Device Details -> Assign Public Key 
- Input Method: Enter Manually 
- Public key format: ES256
- Public key value: content of google_iot_mqtt/src/private_info/<device-id>-ec_public.pem

Users will also be required to configure the following Kconfig options
based on their Google Cloud IOT project.  The following values come
from the Google Cloud Platform itself:

- PROJECT_ID: When you select your project at the top of the UI, it
  should have a "name", and there should be an ID field as well.  This
  seems to be two words and a number, separated by hyphens.
- REGION: The Region shows in the list of registries for your
  registry.  And example is "us-central1".
- REGISTRY_ID: Each registry has an id.  This is a string given when
  creating the registry.
- DEVICE_ID: A name given for each device.  When viewing the table of
  devices, this will be shown.

From these values, the config values can be set using the following
template:

.. code-block: kconfig
  CLOUD_CLIENT_ID="projects/PROJECT_ID/locations/REGION/registries/REGISTRY_ID/devices/DEVICE_ID"
  CLOUD_AUDIENCE="PROJECT_ID"
  CLOUD_SUBSCRIBE_CONFIG="/devices/DEVICE_ID/config"
  CLOUD_PUBLISH_TOPIC="/devices/DEVICE_ID/state"

See `Google Cloud MQTT Documentation
<https://cloud.google.com/iot/docs/how-tos/mqtt-bridge>`_.
