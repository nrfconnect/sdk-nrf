.. _bt_mesh_chat:

Bluetooth Mesh: Chat
####################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Mesh chat sample demonstrates how to use the mesh network to facilitate communication between nodes by text, using the :ref:`bt_mesh_chat_client_model`.

See the subpages for detailed documentation on the sample and its internal model.

.. _mesh_chat_subpages:

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   sample_description.rst
   chat_cli.rst

External flash support
======================

This sample may use an optional external QSPI flash on :zephyr:board:`nrf52840dk/nrf52840` to store mesh settings externally.

Optional external flash variants for :zephyr:board:`nrf52840dk/nrf52840`:

* ``ext_flash``: Enable external flash device.
* ``ext_flash_settings``: Also relocate mesh settings to external flash.

Enable external flash device only:

.. code-block:: console

   west build -p -b nrf52840dk/nrf52840 -- -DFILE_SUFFIX=ext_flash

Enable external flash and relocate mesh settings:

.. code-block:: console

   west build -p -b nrf52840dk/nrf52840 -- -DFILE_SUFFIX=ext_flash -DEXTRA_CONF_FILE=prj_ext_flash_settings.conf

.. note::
   The external flash is not erased during the internal flash erasing procedure.
   See `nRF Util`_ for more information on how to erase the external flash.
   Currently, only the external flash on the ``nrf52840dk/nrf52840`` board is supported.
