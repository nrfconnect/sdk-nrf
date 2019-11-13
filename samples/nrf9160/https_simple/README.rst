.. _https_simple:

nRF9160: HTTPS simple sample
################################

This HTTPS simple sample is a bare-minimal implementation of HTTP communication, showcasing how to
set up a TLS session towards a HTTPS server.

Overview
********

The sample sets up a TLS socket, with minimal configuration, meaning no sec_tag and key pair provided, nor CA root hostname verification.
It then connects to an HTTPS server, and fetches and prints the received data, index.html, before disconnecting.

Requirements
************

* The following nRF device:

  * nRF9160

* .. include:: /includes/spm.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/https_simple`

.. include:: /includes/build_and_run.txt

The sample is built as a non-secure firmware image for the nrf9160_pca10090ns board.
Because of this, it automatically includes the :ref:`secure_partition_manager`.
