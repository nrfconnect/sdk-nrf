.. _aws_fota_sample:

nRF9160: AWS FOTA Sample
########################

The AWS FOTA sample shows how to perform over-the-air firmware updates of an nRF9160 via MQTT and HTTP.

Overview
*********

The sample connects to an AWS MQTT broker and subscribes to related AWS Job topics.
When it receives notification that an update job is available, it retrieves metadata over MQTT, then retrieves the payload over HTTP, and reports back the status when it completes.

See :ref:`lib_aws_fota` for more information.

Requirements
************

* The following development board:

  * nRF9160 DK board (PCA10090)

* .. include:: /includes/spm.txt
* AWS IoT account
* Certificates with correct permissions to the AWS IoT MQTT broker written/flashed into the modem

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/aws_fota`

.. include:: /includes/build_and_run_nrf9160.txt

Prerequisites
*************

The connected device must have the same MQTT Client ID as the thing name created on AWS IoT.
The device needs to have certificates from AWS IoT provisioned onto the device and it needs to have a policy attached to the certificates, and that the certificates are activate in AWS IoT. The process for doing so is described below.

Below is an example of a very permissive policy which should be used for **testing only**.
You can attach it to the certificates if you are unsure what policies are needed for your device.
To create a policy, go to `AWS IoT Console <https://console.aws.amazon.com/iot/home>`_ and select **Secure->Policies->Create**.

.. code-block:: javascript

    {
      "Version": "2012-10-17",
      "Statement": [
          {
            "Effect": "Allow",
            "Action": "iot:*",
            "Resource": "*"
          }
       ]
     }

To create a thing, log in to the `AWS IoT Console <https://console.aws.amazon.com/iot/home>`_ and select **Manage->Things->Create->Create a single thing**.
Provide it with a name which corresponds to your device's MQTT Client ID.
Next, choose to either create certificates or add your own certificates and make sure to **Activate** them.
Also, make sure to download one of the `root CA certificates <https://docs.aws.amazon.com/iot/latest/developerguide/managing-device-certs.html#server-authentication>`_ and the generated certificates if you choose to let AWS generate some for you.

Then, attach the policy you created earlier to the certificates. You must also add the information from the Root CA, Public Certificate, and Private Key into :file:`certificates.h`.

When these steps are done, select ``CONFIG_PROVISION_CERTIFICATES`` in Kconfig.
This will write your new certificates into the modem, to the security tag :c:type:`sec_tag_t` defined in the Kconfig variable ``CONFIG_CLOUD_CERT_SEC_TAG``.

.. warning::

   After provisioning the certificates once, make sure to deselect this option and compile and program the sample again.
   Otherwise, the certificates are stored in the area of readable flash on the device, and they are visible in the compiled binary and on a modem trace.
   This is a security risk for your private key and certificate if the binary is published or leaked, or if the device flash is dumped.
   In production, never store certificates in readable flash on the device or write them to the modem, because these writes are visible on a modem trace.

Usage
*****

1. Configure the host name to be the AWS IoT MQTT Broker endpoint you want to use in menuconfig.
#. Make sure you have provisioned certificates onto your device. See the steps in `prerequisites`_.
#. Flash the sample onto the board.
#. When the sample has successfully connected to the AWS IoT broker, log in to your `AWS IoT Console <https://console.aws.amazon.com/iot/home>`_ and go to **Manage->Jobs->Create**.
#. Then, choose **Create custom job**, provide it with a unique **Job ID**, and select the **Device to update**.
#. Add a **Job file**.

.. note::
   * An example of a **job file** can be found in the documentation of the :ref:`lib_aws_fota` library.
     You can upload it to an `S3 bucket <https://console.aws.amazon.com/s3>`_ on your AWS account and select it from there. If you use the AWS IoT Console, this is the only way to add a job file to a custom job.


7. Choose **Snapshot** as job type and press **Next**.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`lib_aws_fota`
  * ``drivers/lte_link_control``

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

From Zephyr
  * :ref:`MQTT <zephyr:networking_reference>`

In addition, it uses the following samples:

From |NCS|
  * :ref:`secure_partition_manager`
