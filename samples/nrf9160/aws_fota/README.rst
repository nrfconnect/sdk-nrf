.. _aws_fota_sample:

nRF9160: AWS FOTA Sample
########################

The AWS FOTA sample shows how to perform over-the-air firmware updates of an nRF9160 via MQTT and HTTP.

Overview
*********

The sample connects to an AWS MQTT broker and subscribes to a topic.
When it receives notification that an update is available, it retrieves metadata over MQTT, then retrieves the payload over HTTP.

See :ref:`lib_aws_fota` for more information.

Requirements
************

* The following development board:

  * nRF9160 DK board (PCA10090)

* .. include:: /includes/spm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/aws_fota`

.. include:: /includes/build_and_run_nrf9160.txt

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
