module_id_list_all :
{
	__start_module_id_list = .;
	KEEP(*(module_id_list));
	__stop_module_id_list = .;
} GROUP_LINK_IN(ROMABLE_REGION)
