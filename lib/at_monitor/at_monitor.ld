/* AT monitors */
. = ALIGN(4);
_at_monitor_entry_list_start = .;
KEEP(*(SORT_BY_NAME("._at_monitor_entry.*")));
_at_monitor_entry_list_end = .;
