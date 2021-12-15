/* AT monitors, to run in a thread */
. = ALIGN(4);
_at_monitor_entry_list_start = .;
KEEP(*(SORT_BY_NAME("._at_monitor_entry.*")));
_at_monitor_entry_list_end = .;
/* AT monitors, to run in ISR */
. = ALIGN(4);
_at_monitor_isr_entry_list_start = .;
KEEP(*(SORT_BY_NAME("._at_monitor_isr_entry.*")));
_at_monitor_isr_entry_list_end = .;
