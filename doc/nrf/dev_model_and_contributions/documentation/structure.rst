.. _doc_structure:
.. _documentation_sets:

Documentation structure
#######################

.. contents::
   :local:
   :depth: 2

.. doc_structure_start

The documentation consists of several inter-linked documentation sets, one for each repository.

The entry point is the |NCS| documentation that you are currently reading.
The local :doc:`Zephyr documentation <zephyr:index>` is a slightly extended version of the official `Zephyr Project documentation`_, containing some additions specific to Nordic Semiconductor.
The local :doc:`MCUboot documentation <mcuboot:index-ncs>` is a slightly extended version of the official `MCUboot`_ documentation, containing some additions specific to Nordic Semiconductor.

You can navigate these documentation sets by using the table of contents on the left.

.. doc_structure_end

Following are the available documentation sets:

- ``nrf``: |NCS|
- ``nrfx``: nrfx
- ``nrfxlib``: nrfxlib
- ``zephyr``: Zephyr RTOS
- ``mcuboot``: MCUboot
- ``tfm``: Trusted Firmware-M
- ``matter``: Matter
- ``kconfig``: All available Kconfig options in the |NCS|

Since there are links from the |NCS| documentation set into other documentation sets, the documentation is built in a predefined order, also when you :ref:`build it manually <doc_build_steps>`.

.. _doc_structure_versions:

Different set versions
**********************

Some documentation sets are available in different versions.
Use the version drop-down in the *top-left corner of each documentation page* to change the documentation version.

The version drop-down is displayed only if the documentation files are organized in the required folder structure and the documentation is hosted on a web server.
You can :ref:`test the different versions locally <testing_versions>` when you build the documentation.
