.. _hwmv1_to_v2_migration:

Migrating to the current hardware model
#######################################

The *hardware model* refers to how SoCs and boards are named, defined, and used.
Starting with |NCS| version 2.7.0, this model has been completely updated.

.. note::
   The current hardware model was introduced in the Zephyr Project shortly after the release of 3.6.0.

This section describes how to migrate from the hardware model used in |NCS| versions 2.6.x and earlier (commonly referred to as *hardware model v1*, or *hwmodelv1*) to the current one (commonly referred to as *hardware model v2*).

Model overview
**************

The current hardware model adds support for the following features:

* Support for multi-core, multi-architecture Asymmetrical Multi Processing (AMP) SoCs.
* Support for multi-SoC boards.
* Support for reusing the SoC and board Kconfig trees outside of the Zephyr and |NCS| build system.
* Support for advanced use cases with :ref:`zephyr:sysbuild`.
* Removal of all existing arbitrary and inconsistent uses of Kconfig and folder names.

For more information about the rationale, development, and concepts behind the new model, see the `Zephyr original issue <Zephyr issue #51831>`_, the `Zephyr original Pull Request <Zephyr pull request #50305>`_,.
For the complete set of changes introduced, consult the `hardware model v2 commit <Zephyr commit 8dc3f8>`_.

Some non-critical features, enhancements, and improvements of the new hardware model are still in development.
Check the `Zephyr hardware model v2 enhancements issue <Zephyr issue #69546>`_ for a complete list.

Out-of-tree boards
******************

The |NCS| provides full backwards compatibility for any existing out-of-tree boards.

As such, the migration from hardware model v1 to hardware model v2 is optional for |NCS| users that also have out-of-tree boards.
If you choose not to convert an out-of-tree board, you will receive a warning when building your application, informing you that the board definition is in a format compatible with hardware model v1.

Board migration
***************

To convert your board from hardware model v1 to v2, consult the following pages:

* :ref:`zephyr:hw_support_hierarchy` and the subsequent sections to understand the model in detail.
* :ref:`zephyr:hw_support_hierarchy` and the subsequent sections to understand the hardware model in detail.
* The `ncs-example-application conversion commit <ncs-example-application commit f9f2da>`_ as an example of how to port a simple board.
* The `hardware model v2 commit <Zephyr commit 8dc3f8>`_, containing the full conversion of all existing boards from the old to the current model, to use as a complete conversion reference.

Conversion script
-----------------

There is a `conversion script <hwmv2 conversion script>`_ to help you in the migration process.
It has built-in help text documenting all the parameters it takes.
The following example shows an invocation of the script converting a hypothetical out-of-tree board ``plank`` based on the nRF52840 and having ``acme`` as vendor.

.. code-block::
   :class: highlight

   $ cd my-repo
   $ python ../zephyr/scripts/utils/board_v1_to_v2.py --board-root boards/ -b plank -s nrf52840 -v acme


After running this command, the script converts the board from the previous hardware model to the current one, creating a board in the new format in :file:`my-repo/boards/acme/plank`.
The script assumes that you have a repository named :file:`my-repo`, having a :file:`boards` folder located at the same level as the :file:`nrf` folder.
