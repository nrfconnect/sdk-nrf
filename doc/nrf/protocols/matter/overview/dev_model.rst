.. _ug_matter_overview_dev_model:

Matter development model and compatible ecosystems
##################################################

.. contents::
   :local:
   :depth: 2

The Matter stack is developed on the official `Matter GitHub repository`_ and is open-sourced.

The Matter stack implementation contains separation between platform-agnostic and platform-dependent functionality.
The open-source implementation offers ports for several resource-constrained, embedded SoCs as well as POSIX-based platforms.

Matter strives for reusing technologies from market-proven solutions, such as Apple HomeKit or Google Weave.
Wi-Fi and :ref:`ug_thread` are its main wireless connectivity protocols that offer seamless integration with other IPv6-based networks and are application-layer agnostic.
Bluetooth® LE can be used for commissioning of the Matter accessories, and QR codes and NFC tags can be used to initiate the commissioning.

Matter in the |NCS|
*******************

The |NCS| provides full toolchain for Linux, macOS, and Windows, and is built on top of the Zephyr RTOS.
It includes west for managing repositories, toolchain manager for managing toolchain, Kconfig for feature configuration, and Devicetree for board description.
Finally, it integrates the OpenThread and Wi-Fi stacks, both of which can work in a multiprotocol scenario with the integrated Bluetooth® LE stack.

Nordic Semiconductor integrates the Matter stack in the |NCS| using a `dedicated Matter fork`_.
The official Matter repository is fetched into the fork and the fork is included in the |NCS| as a Zephyr module, including files deployed in the :ref:`matter_index` tab.
The fork is maintained and verified as a part of the nRF Connect SDK release process as an :ref:`OSS repository <dm_repo_types>`.

For more information about Matter architecture and Matter in the |NCS|, read :ref:`ug_matter_overview_architecture_integration`.

.. _ug_matter_overview_dev_model_ecosystems:

Compatibility with commercial ecosystems
****************************************

One of the key features of the Matter protocol is the interoperability of different ecosystems it provides.
Implementing support for Matter enables the :ref:`ug_matter_overview_multi_fabrics` and allows different vendor products to co-exist within the same Matter network.

The Matter stack in the |NCS| will work with any commercial Matter ecosystem as long as these ecosystems are compatible with the official Matter implementation (for example `Apple Home <Apple Home integration with Matter_>`_, `Google Home <Google Home integration with Matter_>`_, or `Samsung SmartThings <Samsung SmartThings integration with Matter_>`_).

For an example of interoperability of some commercial ecosystems, see :ref:`ug_matter_gs_ecosystem_compatibility_testing`.
