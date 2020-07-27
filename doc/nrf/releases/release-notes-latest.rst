.. _ncs_release_notes_latest:

Changes in |NCS| v1.3.99
########################

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest release, are tracked in this file.
Note that this file is a work in progress and might not cover all relevant changes.


Change log
**********

See the `master branch section in the change log`_ for a list of the most important changes.

sdk-nrfxlib
===========

See the change log for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for the most current information.

sdk-zephyr
==========

The Zephyr fork in |NCS| contains all commits from the upstream Zephyr repository up to and including ``4ef29b34e3``, plus some |NCS| specific additions.

The following list summarizes the most important changes inherited from upstream Zephyr:

* Kernel

  * The ``CONFIG_KERNEL_DEBUG`` Kconfig option, which was used to enable ``printk()`` based debugging of the kernel internals, has been removed.
    The kernel now uses the standard Zephyr logging API at DBG log level for this purpose.
    The logging module used for the kernel is named ``os``.

* Networking:

  * LwM2M:

    * Fixed a bug where a FOTA socket was not closed after the download (PULL mode).
    * Added a Kconfig option :option:`CONFIG_LWM2M_SECONDS_TO_UPDATE_EARLY` that specifies how long before the time-out the Registration Update will be sent.
    * Added ObjLnk resource type support.

  * MQTT:

    * The ``utf8`` pointer in the :c:type:`mqtt_utf8` struct is now const.
    * The default ``clean_session`` value is now configurable with Kconfig (see :option:`CONFIG_MQTT_CLEAN_SESSION`).

  * OpenThread:

    * Updated the OpenThread revision to upstream commit e653478c503d5b13207b01938fa1fa494a8b87d3.
    * Implemented a missing ``enable`` API function for the OpenThread interface.
    * Cleaned up the OpenThread Kconfig file.
      OpenThread dependencies are now enabled automatically.
    * Allowed the application to register a callback function for OpenThread state changes.
    * Reimplemented the logger glue layer for better performance.
    * Updated the OpenThread thread priority class to be configurable.
    * Added several Kconfig options to customize the OpenThread stack.

  * Socket offloading:

    * Removed dependency to the :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` configuration option.

The following list contains |NCS| specific additions:

*
