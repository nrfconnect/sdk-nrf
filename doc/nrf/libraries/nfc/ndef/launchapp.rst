.. _nfc_launch_app:

Launch App message and records
##############################

.. contents::
   :local:
   :depth: 2

Launch App library provides a way to create NFC messages capable of launching Android or iOS applications.

Overview
********

Launch App message contains two records: URI record and Android Launch App record.
Typically, the URI record contains a Universal Link used by iOS and the Android record contains an Android package name.
The Universal Link is a link associated to the application.

Implementation
**************

The :ref:`nfc_launchapp_rec` module provides functions for creating the record, and the :ref:`nfc_launchapp_msg` module provides functions for creating and encoding the message.

The following code snippets show how to generate a Launch App message.

1. Define the Universal Link string, Android package name string and create a buffer for the message:

   .. literalinclude:: ../../../../../samples/nfc/record_launch_app/src/main.c
       :language: c
       :start-after: include_startingpoint_pkg_def_launchapp_rst
       :end-before: include_endpoint_pkg_def_launchapp_rst

#. Create the Launch App message:

   .. code-block:: c

      int err;

      err = nfc_launchapp_msg_encode(android_package_name,
        			  sizeof(android_package_name),
        			  universal_link,
        			  sizeof(universal_link),
        			  ndef_msg_buf,
        			  &len);

      if (err < 0) {
           printk("Cannot encode message!\n");
           return err;
      }

   Provide the following parameters:

    * Android package name string
    * Length of Android package name string
    * Universal Link string
    * Length of the Universal Link string
    * Message buffer
    * Size of the available memory in the buffer


Supported features
******************
The library supports encoding AAR (Android Application Record) and Universal Links into NFC message.

Samples using the library
*************************

The :ref:`record_launch_app` sample uses this library.

Dependencies
************
* :ref:`nfc_uri_msg`
* :ref:`nfc_ndef_msg`
* :ref:`nfc_ndef_record`

API documentation
*****************

.. _nfc_launchapp_msg:

Launch App messages
===================

| Header file: :file:`include/nfc/ndef/launchapp_msg.h`
| Source file: :file:`subsys/nfc/ndef/launchapp_msg.c`

.. doxygengroup:: nfc_launchapp_msg
   :project: nrf
   :members:

.. _nfc_launchapp_rec:

Launch App records
===================

| Header file: :file:`include/nfc/ndef/launchapp_rec.h`
| Source file: :file:`subsys/nfc/ndef/launchapp_rec.c`

.. doxygengroup:: nfc_launchapp_rec
   :project: nrf
   :members:
