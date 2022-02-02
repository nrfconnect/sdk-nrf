event_manager_proxy_event_type_size_section 0 (DSECT) :
{
	KEEP(*(event_manager_proxy_event_type_size));
}

event_manager_proxy_event_type_pointer_size_section 0 (DSECT) :
{
	KEEP(*(event_manager_proxy_event_type_pointer_size));
}

event_manager_proxy_array : ALIGN_WITH_INPUT
{
	_event_manager_proxy_array_list_start = .;
	event_manager_proxy_array = .;
	. = . + (_event_type_list_end - _event_type_list_start)
		/ SIZEOF(event_manager_proxy_event_type_size_section)
		* SIZEOF(event_manager_proxy_event_type_pointer_size_section)
		* CONFIG_EVENT_MANAGER_PROXY_CH_COUNT;
	_event_manager_proxy_array_list_end = .;
} GROUP_LINK_IN(RAMABLE_REGION)
