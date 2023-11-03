.. _zboss:

ZBOSS Zigbee stack
##################

The |NCS|'s :ref:`nrf:ug_zigbee` stack uses ZBOSS – a portable, high-performance Zigbee software protocol stack that allows for interoperability, customizing, testing, and optimizing of your Zigbee solution.

The nrfxlib repository contains the following versions of the ZBOSS libraries:

* *Production* version that contains the latest stable ZBOSS libraries.
  This version is enabled with the :kconfig:option:`CONFIG_ZIGBEE_LIBRARY_PRODUCTION` Kconfig option and its files are located in the :file:`zboss/production/` directory.
  This version is enabled by default.

  The production libraries fully conform to the certification, but they are not necessarily certified.

* *Development* version that contains the latest version of ZBOSS libraries, with experimental features included.
  This version is enabled with the :kconfig:option:`CONFIG_ZIGBEE_LIBRARY_DEVELOPMENT` Kconfig option and its files are located in the :file:`zboss/development/` directory.

  This version might not conform to the latest Zigbee Pro R22 test specification.
  There is no guarantee that the library conforms to the certification.

For information about additional configuration of these libraries and their certification status, see :ref:`zboss_configuration`.
For detailed documentation of the ZBOSS API for these versions and instructions on how to use it, check the ZBOSS API documentation using the following banner.

.. rst-class:: doc-link zboss-box
..

    .. rst-class:: doc-link-image

    .. image:: doc/images/zoi-logo.png
       :target: `ZBOSS API documentation`_

    .. rst-class:: doc-link-text

    `ZBOSS API docs <ZBOSS API documentation_>`_


.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   zboss_configuration
   zboss_certification
   CHANGELOG
