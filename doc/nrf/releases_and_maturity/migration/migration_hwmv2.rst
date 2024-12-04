:orphan:

.. _hwmv1_to_v2_migration:

Migrating to the current hardware model
#######################################

.. contents::
   :local:
   :depth: 2

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
* Support for advanced use cases with :ref:`configuration_system_overview_sysbuild`.
* Removal of all existing arbitrary and inconsistent uses of Kconfig and folder names.

For more information about the rationale, development, and concepts behind the new model, see the following links:

* `Zephyr original issue <Zephyr issue #51831_>`_
* `Zephyr original pull request <Zephyr pull request #50305_>`_

For the complete set of changes introduced, consult the `hardware model v2 commit <Zephyr commit 8dc3f8_>`_.

Some non-critical features, enhancements, and improvements of the new hardware model are still in development.
Check the Zephyr `hardware model v2 enhancements issue <Zephyr issue #69546_>`_ for a complete list.

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
* The `ncs-example-application conversion commit <ncs-example-application commit f9f2da_>`_ as an example of how to port a simple board.
* The `hardware model v2 commit <Zephyr commit 8dc3f8_>`_, containing the full conversion of all existing boards from the old to the current model, to use as a complete conversion reference.

.. note::
   Board names must be unique, so you cannot have boards in hardware model v1 and v2 description with the same name.
   If you decide to create a new board in hardware model v2 instead of converting an existing board, ensure that corresponding board config fragments and devicetree overlays for the new boards are also created and any tests for the board in Kconfig, build system, and source code are correctly updated.

Conversion script
-----------------

There is a `conversion script <hwmv2 conversion script_>`_ to help you in the migration process.
It has built-in help text documenting all the parameters it takes.

.. note::
   The script may not be able to automatically convert all boards.
   Boards based on multicore SoCs (nRF53) or with non-secure targets (nRF53/nRF91) will need manual changes.

The following example shows an invocation of the script converting a hypothetical out-of-tree board ``plank_nrf52840`` based on the nRF52840 and having ``acme`` as vendor.
The command must be run from the :ref:`toolchain environment <using_toolchain_environment>`.

.. code-block::
   :class: highlight

   cd my-repo
   python ../zephyr/scripts/utils/board_v1_to_v2.py --board-root . -b plank_nrf52840 -n plank -s nrf52840 -g acme -v acme


After running this command, the script converts the board from the previous hardware model to the current one, creating a board in the new format in :file:`my-repo/boards/acme/plank`.
The script assumes that you have a repository named :file:`my-repo`, having a :file:`boards` folder located at the same level as the :file:`nrf` folder.

The conversion script cannot handle board variants.
If your board uses variants, such as ``ns`` (non-secure), you must manually define them after running the conversion script.

Verify the changes made by the script and test your board.

When you are satisfied with the new board description, commit the changes to your repository.

.. note::
   The script will remove the board in hardware model v1 description, because board names must be unique.
   Also, a given folder can only contain a board in either hardware model v1 or v2 format.
