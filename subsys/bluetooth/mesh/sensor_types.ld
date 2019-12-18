SECTION_DATA_PROLOGUE(bt_mesh_sensor_types_sections,,SUBALIGN(4))
{
	_bt_mesh_sensor_type_list_start = .;
#ifdef CONFIG_BT_MESH_SENSOR_ALL_TYPES
	KEEP(*(SORT_BY_NAME("._bt_mesh_sensor_type.static.*")));
#else
	*(SORT_BY_NAME("._bt_mesh_sensor_type.static.*"));
#endif
	_bt_mesh_sensor_type_list_end = .;
} GROUP_LINK_IN(ROMABLE_REGION)
