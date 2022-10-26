. = ALIGN(4);
_nrf_modem_at_cmd_filter_list_start = .;
KEEP(*(SORT_BY_NAME("._nrf_modem_at_cmd_filter.*")));
_nrf_modem_at_cmd_filter_list_end = .;
