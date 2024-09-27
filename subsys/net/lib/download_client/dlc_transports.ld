/* DLC transports */
. = ALIGN(4);
_dlc_transport_entry_list_start = .;
KEEP(*(SORT_BY_NAME("._dlc_transport_entry.*")));
_dlc_transport_entry_list_end = .;
