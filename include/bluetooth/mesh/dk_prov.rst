.. _bt_mesh_dk_prov:

Bluetooth Mesh provisioning handler for Nordic DKs
##################################################

This application-side module is a basic implementation of the provisioning process handling for Development Kits from Nordic Semiconductor.
It supports four types of Out Of Band (OOB) authentication methods and uses the Hardware Information driver to generate a deterministic UUID to uniquely represent the device.
For more information about provisioning in Bluetooth Mesh, see the :ref:`zephyr:bluetooth_mesh_provisioning` page in Zephyr.

Used primarily in :ref:`Bluetooth Mesh sample applications <ble_samples>`, this handler acts as a reference implementation for the application-specific part of provisioning.
It is enabled with the :option:`CONFIG_BT_MESH_DK_PROV` option and by calling :cpp:func:`bt_mesh_dk_prov_init` in main.

The handling of the OOB authentication methods is closely tied to the hardware parameters of the Development Kits from Nordic Semiconductor.
For deployments with custom hardware, do not extend this module.
Instead, create a similar custom module that is tailored specifically to your board capabilities.

Authentication methods
======================

The provisioning handler provides the following OOB authentication methods during provisioning:

Output: Display number
    The Display number OOB method requires access to the application log through a serial connection.
    If selected, the node will print an authentication number in its serial log.
    Enter this number into the provisioner.

    Enabled by :option:`CONFIG_BT_MESH_DK_PROV_OOB_LOG`.

Output: Display string
    The Display string OOB method requires access to the application log through a serial connection.
    If selected, the node will print an authentication string in its serial log.
    Enter this number into the provisioner.

    Enabled by :option:`CONFIG_BT_MESH_DK_PROV_OOB_LOG`.

Output: Blink
    The Blink OOB method blinks the LEDs a certain number of times.
    Enter the number of blinks into the provisioner.
    The blinks occur at a frequency of two blinks per second, and the sequence is repeated every three seconds.

    Enabled by :option:`CONFIG_BT_MESH_DK_PROV_OOB_BLINK`.

Input: Push button
    The Push button OOB method requires the user to press a button the number of times specified by the provisioner.
    After three seconds of inactivity, the button count is sent to the provisioner.

    Enabled by :option:`CONFIG_BT_MESH_DK_PROV_OOB_BUTTON`.

    .. note::
        It is also possible for the provisioner to select "No authentication" and skip the OOB authentication.
        This is not recommended, as it compromises the Mesh network security.

UUID
====

The provisioning handler uses :cpp:func:`hwinfo_get_device_id` from the ``hwinfo`` driver to generate a unique, deterministic UUID for each device.
The UUID is used in the unprovisioned beacon to identify the device.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/dk_prov.h`
| Source files: :file:`subsys/bluetooth/mesh/dk_prov.c`

.. doxygengroup:: bt_mesh_dk_prov
   :project: nrf
   :members:
