SECTION_DATA_PROLOGUE(bt_mesh_scene_entries_sections,,SUBALIGN(4))
{
#ifdef CONFIG_BT_MESH_SCENE_SRV
	_bt_mesh_scene_entry_sig_list_start = .;
	KEEP(*(SORT_BY_NAME("._bt_mesh_scene_entry.static.bt_mesh_scene_entry_sig_*")));
	_bt_mesh_scene_entry_sig_list_end = .;
	_bt_mesh_scene_entry_vnd_list_start = .;
	KEEP(*(SORT_BY_NAME("._bt_mesh_scene_entry.static.bt_mesh_scene_entry_vnd_*")));
	_bt_mesh_scene_entry_vnd_list_end = .;
#else
	/* Will be discarded by linker, unless they're explicitly referenced by
	 * some other module:
	 */
	*(SORT_BY_NAME("._bt_mesh_scene_entry.static.*"));
#endif
} GROUP_LINK_IN(ROMABLE_REGION)
