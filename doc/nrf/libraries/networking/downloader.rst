.. _lib_downloader:

Downloader
##########

.. contents::
   :local:
   :depth: 2

You can use the downloader library to download files from a server.

Overview
********

The download is carried out in a separate thread and the application receives events such as :c:enumerator:`DOWNLOADER_EVT_FRAGMENT` that contain the data fragments as the download progresses.
When the download completes, the library sends the :c:enumerator:`DOWNLOADER_EVT_DONE` event to the application.

Protocols
=========

The library supports HTTP, HTTPS (TLS 1.2), CoAP, and CoAPS (DTLS 1.2) over IPv4 and IPv6.
If other protocols are required, they can be added by the application.
See :file:`downloader_transport.h` for details.
The protocol used for the download is specified in the beginning of the ``URI`` or ``host``.
Use ``http://`` for HTTP, ``https://`` for HTTPS, ``coap://`` for CoAP, and ``coaps://`` for CoAPS.
If no protocol is specified, the downloader defaults to HTTP or HTTPS, depending on the server security configuration.

.. _downloader_https:

HTTP and HTTPS (TLS 1.2)
------------------------

When downloading using HTTP, the library sends only one HTTP request to the server and receives only one HTTP response.

When downloading using HTTPS with an nRF91 Series device, it is carried out through `Content-Range requests (IETF RFC 7233)`_ due to memory constraints that limit the maximum HTTPS message size to two kilobytes.
The library thus sends and receives as many requests and responses as the number of fragments that constitute the download.
For example, to download a file of 47 kilobytes with a fragment size of 2 kilobytes, a total of 24 HTTP GET requests are sent.
The download can also be carried out through fragments by specifying the :c:member:`downloader_host_cfg.range_override` field of the host configuration.

CoAP and CoAPS (DTLS 1.2)
-------------------------

The CoAP feature is disabled by default.
You can enable it using the :kconfig:option:`CONFIG_DOWNLOADER_TRANSPORT_COAP` Kconfig option.
When downloading from a CoAP server, the library uses the CoAP block-wise transfer.

Configuration
*************

The configuration of the library depends on the protocol you are using.

Configuring HTTP and HTTPS (TLS 1.2)
====================================

Make sure that the buffer provided to the downloader is large enough to accommodate the entire HTTP header of the request.
Ensure that the values of the :kconfig:option:`CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE` and :kconfig:option:`CONFIG_DOWNLOADER_MAX_FILENAME_SIZE` Kconfig options are large enough for your host and filenames, respectively.

When using HTTPS the application must provision the TLS credentials and pass the security tag to the library through the :c:struct:`downloader_host_cfg` structure.
To provision a TLS certificate to the modem, use :c:func:`modem_key_mgmt_write` and other :ref:`modem_key_mgmt` APIs.

Configuring CoAP and CoAPS (DTLS 1.2)
=====================================

Make sure the buffer provided to the downloader is large enough to accommodate the entire CoAP header and the CoAP block.
You can configure the CoAP block size using the :c:func:`downloader_transport_coap_set_config` function.
Ensure that the values of the :kconfig:option:`CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE` and :kconfig:option:`CONFIG_DOWNLOADER_MAX_FILENAME_SIZE` Kconfig options are large enough for your host and filenames, respectively.

When using CoAPS the application must provision the TLS credentials and pass the security tag to the library through the :c:struct:`downloader_host_cfg` structure.

When you have modem firmware v1.3.5 or newer, you can use the DTLS Connection Identifier feature in this library by setting the ``cid`` flag in the :c:struct:`downloader_host_cfg` structure.

Usage
*****

To initialize the library, call the :c:func:`downloader_init` function as follows:

.. code-block:: c

   int err;

   static int dl_callback(const struct downloader_evt *event);
   char dl_buf[2048];
   struct downloader dl;
   struct downloader_cfg dl_cfg = {
         .callback = dl_callback,
         .buf = dl_buf,
         .buf_size,
   };

   err = downloader_init(&dl, &dl_cfg);
   if (err) {
         printk("downloader init failed, err %d\n", err);
   }

To deinitialize the library, call the :c:func:`downloader_deinit` function as follows:

.. code-block:: c

   int err;
   struct downloader dl;

   /* downloader is initialized */

   err = downloader_deinit(&dl);
   if (err) {
         printk("downloader deinit failed, err %d\n", err);
   }

This will free up the resources used by the library.

The following snippet shows how to download a file using HTTPS:

.. code-block:: c


   int err;
   int dl_res;

   static int dl_callback(const struct downloader_evt *event) {
         switch (event->id) {
         case DOWNLOADER_EVT_FRAGMENT:
               printk("Received fragment, dataptr: %p, len %d\n",
                      event->fragment.buf, event->fragment.len);
               return 0;
         case DOWNLOADER_EVT_ERROR:
               printk("downloader error: %d\n", event->error);
               dl_res = event->error;
               return 0;
         case DOWNLOADER_EVT_DONE:
               printk("downloader done\n");
               dl_res = 0;
               return 0;
         case DOWNLOADER_EVT_STOPPED:
               printk("downloader stopped\n");
               k_sem_give(&dl_sem);
               return 0;
         case DOWNLOADER_EVT_DEINITIALIZED:
               printk("downloader deinitialized\n");
               return 0;
         }
   }

   char dl_buf[2048];
   struct downloader dl;
   struct downloader_cfg dl_cfg = {
         .callback = dl_callback,
         .buf = dl_buf,
         .buf_size,
   };

   int sec_tags[] = {1, 2, 3};

   struct downloader_host_cfg dl_host_cfg = {
         .sec_tag_list = sec_tags,
         .sec_tag_count = ARRAY_SIZE(sec_tags),
         /* This will disconnect the downloader from the server when the download is complete */
         .keep_connection = false,
   };

   struct downloader_transport_http_cfg dl_transport_http_cfg = {
         .sock_recv_timeo_ms = 60 * MSEC_PER_SEC,
   };

   err = downloader_init(&dl, &dl_cfg);
   if (err) {
         printk("downloader init failed, err %d\n", err);
   }

   err = downloader_transport_http_set_config(&dl, &dl_transport_http_cfg);
   if (err) {
         printk("failed to set http transport params failed, err %d\n", err);
   }

   err = downloader_get(&dl, &dl_host_cfg, "https://myserver.com/path/to/file.txt");
   if (err) {
         printk("downloader start failed, err %d\n", err);
   }

   /* Wait for download to complete */
   k_sem_take(&dl_sem, K_FOREVER);

   err = downloader_deinit(&dl);
   if (err) {
         printk("downloader deinit failed, err %d\n", err);
   }

Limitations
***********

The following limitations apply to this library:

nRF91 Series TLS limitation
===========================

The nRF91 Series modem has a size limit for receiving TLS packages.
The size limit depends on modem internals and is around 2 kB.
See modem firmware release notes for details.
The library  asks the server for a content-range which must be supported by the host server when using HTTPS with the nRF91 Series devices.

The content range is set by the :c:member:`downloader_host_cfg.range_override` configuration in the download client configuration.
If the configuration is not set, a default value will be used for the nRF91 Series devices when using HTTPS.

The fragment size must be set so that the TLS package does not exceed the modem limit.
The TLS package size is dependent on the HTTP header and payload size.
The HTTP header size is dependent on the server in use.
When meeting this limitation, the downloader attempts to reduce the content-range in order to fill the TLS size requirements.
If the requirements cannot be met, the downloader fails with error ``-EMSSIZE``.

.. note::
   If you are experiencing this issue on a deployed product, reducing the HTTP header size responded by the server can also resolve this issue.

CoaP block size
===============

Due to internal limitations, the maximum CoAP block size is 512 bytes.

API documentation
*****************

| Header file: :file:`include/downloader.h`, :file:`include/downloader_transport.h`, :file:`include/downloader_transport_http.h`, :file:`include/downloader_transpot_coap.h`
| Source files: :file:`subsys/net/lib/downloader/src/`

.. doxygengroup:: downloader
