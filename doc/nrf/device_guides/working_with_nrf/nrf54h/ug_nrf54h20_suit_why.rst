:orphan:

.. _ug_nrf54h20_suit_why:

Why SUIT was selected as a bootloader and DFU procedure
#######################################################

.. contents::
   :local:
   :depth: 2

The nRF54H Series contains multiple CPU cores and images as opposed to other products by Nordic Semiconductor, such as the nRF52 Series.
Multiple CPU cores add more possibilities for development and complexity for operations such as performing a DFU.
This requires a DFU procedure that can address the needs of a single device containing multiple cores and multiple images.

Therefore, the SUIT procedure was selected due to its features:

* Script-based systems that allow you to customize the instructions for installation and invocation

* Dependency management between multiple executable images

* Support for multiple signing authorities that will be required for each dependency

* Customization for the allocation of upgradeable elements

.. note::

   The term "invocation" is used in this document to mean "booting procedure".

Script-based systems
********************

SUIT features a script-based system.
This contains high-level instructions collected into *command sequences*.
SUIT's command sequences allow you to express certain sequences for installation and invocation in an imperative way.
This offers flexibility in cases where, for example, you want to minimize the size of a download within the DFU lifecycle to increase flash efficiency.

This allows for extensive management of dependencies between components, meaning you can execute different logic on different groups of components.
For example, the location of a slot or component does not have to be static and can be changed within the manifest.

The script-based system also allows you to direct the manifest to download only the necessary, specified data.
You can do this, for example, by checking what is already in the device receiving the DFU or by checking the version number of the existing firmware.
This saves on both transfer cost and memory on the device.

SUIT topology
*************

SUIT features a topology which allows for dependency management between multiple executable images.
Additionally, the SUIT topology allows you to provide multiple signing authorities that will be required for each dependency.

For information on the SUIT topology used by Nordic Semiconductor's implementation of SUIT, see the :ref:`ug_nrf54h20_suit_hierarchical_manifests` page.

Allocation of components
************************

SUIT itself does not decide the component allocation, rather it can be defined in the manifest.
This is because their location is flexible and there are some options available to change the location of a specific image within the manifest.

See the :ref:`ug_nrf54h20_suit_compare_other_dfu` page for further information on why SUIT was selected for DFU procedures for the nRF54H Series of SoC.
