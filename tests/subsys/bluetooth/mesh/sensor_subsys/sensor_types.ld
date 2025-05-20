SECTION_DATA_PROLOGUE(bt_mesh_sensor_types_sections,,SUBALIGN(4))
{
	_bt_mesh_sensor_type_list_start = .;
	KEEP(*(SORT_BY_NAME("._bt_mesh_sensor_type.static.*")));
	_bt_mesh_sensor_type_list_end = .;
} GROUP_LINK_IN(ROMABLE_REGION)
