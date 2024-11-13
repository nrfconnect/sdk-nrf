/* DL transports */
. = ALIGN(4);
_dl_transport_entry_list_start = .;
KEEP(*(SORT_BY_NAME("._dl_transport_entry.*")));
_dl_transport_entry_list_end = .;
