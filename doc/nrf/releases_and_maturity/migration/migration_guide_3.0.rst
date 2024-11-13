.. _migration_3.0:

Migration guide for |NCS| v3.0.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v2.9.0 to |NCS| v3.0.0.

.. HOWTO
   Add changes in the following format:
   Component (for example, application, sample or libraries)
   *********************************************************
   .. toggle::
      * Change1 and description
      * Change2 and description

.. _migration_3.0_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Samples and applications
========================

This section describes the changes related to samples and applications.

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|

.. _migration_3.0_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Samples and applications
========================

This section describes the changes related to samples and applications.

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

Download client
---------------

.. toggle::

   * The :ref:`lib_download_client` library has been deprecated in favor of the :ref:`lib_downloader` library and will be removed in a future |NCS| release.

     You can follow this guide to migrate your application to use the :ref:`lib_downloader` library.
     This will reduce the footprint of the application and will decrease memory requirements on the heap.

     To replace :ref:`lib_download_client` with the :ref:`lib_downloader`, complete the following steps.

     1. Kconfig options:

         * Replace:

            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_MAX_FILENAME_SIZE` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_STACK_SIZE` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_STACK_SIZE` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_SHELL` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_SHELL` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_TCP_SOCK_TIMEO_MS` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_HTTP_TIMEO_MS` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_COAP_MAX_RETRANSMIT_REQUEST_COUNT` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_COAP_MAX_RETRANSMIT_REQUEST_COUNT` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_COAP_BLOCK_SIZE` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_COAP_BLOCK_SIZE_512` Kconfig option.

         * Remove:

            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_BUF_SIZE` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_CID` Kconfig option.

         * Add:

            * The :kconfig:option:`CONFIG_DOWNLOADER_TRANSPORT_COAP` Kconfig option to enable CoAP support.
            * The :kconfig:option:`CONFIG_NET_IPV4` Kconfig option to enable IPv4 support.
            * The :kconfig:option:`CONFIG_NET_IPV6` Kconfig option to enable IPv6 support.

     #. Replace header files:

        * Remove:

          .. code-block:: C

             #include <net/download_client.h>

        * Add:

          .. code-block:: C

             #include <net/downloader.h>

     #. Replace download client initialization:

        * Remove:

          .. code-block:: C

              static struct download_client dlc;
              static int callback(const struct download_client_evt *event);

              download_client_init(&dlc, callback)

        * Add:

         .. code-block:: C

            static struct downloader dl;
            static int callback(const struct downloader_evt *event);
            static char dl_buf[2048]; /* Use buffer size set by CONFIG_DOWNLOAD_CLIENT_BUF_SIZE previously */
            static struct downloader_cfg dl_cfg = {
               .callback = callback,
               .buf = dl_buf,
               .buf_size = sizeof(dl_buf),
            };

            downloader_init(&dl, &dl_cfg);

     #. Update download client callback:

        * Replace:

            * :c:enumerator:`DOWNLOAD_CLIENT_EVT_FRAGMENT` event with :c:enumerator:`DOWNLOADER_EVT_FRAGMENT`.
            * :c:enumerator:`DOWNLOAD_CLIENT_EVT_ERROR` event with :c:enumerator:`DOWNLOADER_EVT_ERROR`.
            * :c:enumerator:`DOWNLOAD_CLIENT_EVT_DONE` event with :c:enumerator:`DOWNLOADER_EVT_DONE`.

        * Remove:

            * :c:enumerator:`DOWNLOAD_CLIENT_EVT_CLOSED` event.

        * Add:

            * :c:enumerator:`DOWNLOADER_EVT_STOPPED` event.
            * :c:enumerator:`DOWNLOADER_EVT_DEINITIALIZED` event.

     #. Server connect and disconnect:

        * The :c:func:`download_client_disconnect` function is not ported to the new downloader.
          The downloader is expected to connect when the download begins.
          If the ``keep_connection`` flag is set in the host configuration the connection persists after the download completes or is aborted by the :c:func:`downloader_cancel` function.
          In this case, the downloader is disconnected when it is deinitialized by the :c:func:`downloader_deinit` function.


     #. Replace file download:

        We show the changes for the :c:func:`download_client_start` function here, though the required work is
        similar to the :c:func:`download_client_get` function.

        * Remove:

        .. code-block:: C

           int err;
           const struct download_client_cfg dlc_config = {
              ...
           };

           err = download_client_set_host(&dlc, dl_host, &dlc_config);

           err = download_client_start(&dlc, dl_file, offset);

        * Add:

        .. code-block:: C

           /* Note: All configuration of the downloader is done through the config structs.
            * The downloader struct should not be modified by the application.
            */

           static struct downloader_host_cfg dl_host_cfg = {
                   ...
                   /* Note:
                    * .frag_size_override is replaced by .range_override.
                    * .set_tls_hostname is replaced by .set_native_tls.
                    * dlc.close_when_done is moved here and inverted(.keep_connection).
                    * Set .cid if CONFIG_DOWNLOAD_CLIENT_CID was enabled in the download client.
                    */
           };

           int err = downloader_get_with_host_and_file(&dl, &dl_host_cfg, dl_host, dl_file, offset);

        .. note::
           The new downloader has an API to download the file using the URI directly.

     #. [optional] Deinitialize the downloader after use:

        The new downloader can be deinitialized to free its resources.
        If another download is required later on, a new downloader instance needs to be initialized.

        * Add:

        .. code-block:: C

           err = downloader_deinit(&dl);
