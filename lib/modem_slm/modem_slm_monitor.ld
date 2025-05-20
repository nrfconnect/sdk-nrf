/* AT monitors */
. = ALIGN(4);
_slm_monitor_entry_list_start = .;
KEEP(*(SORT_BY_NAME("._slm_monitor_entry.*")));
_slm_monitor_entry_list_end = .;
