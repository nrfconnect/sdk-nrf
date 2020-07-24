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

The following list contains |NCS| specific additions:

*
