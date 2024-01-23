.. ug_bt_mesh_overview_coexistence:

Bluetooth Mesh and Bluetooth LE coexistence
###########################################

Bluetooth Mesh uses the preset local identity value.
For more information about the advertisement identity, see :ref:`zephyr:bluetooth_mesh_adv_identity`.

The :ref:`bluetooth_ble_peripheral_lbs_coex` sample demonstrates how to perform correct local identity allocation for Bluetooth LE purposes to avoid identity conflict with Bluetooth Mesh.
See the file :file:`samples/bluetooth/mesh/ble_pripheral_lbs_coex/src/lb_service_handler.c` for details.
