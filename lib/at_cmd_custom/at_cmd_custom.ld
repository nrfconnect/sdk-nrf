. = ALIGN(4);
_nrf_modem_at_cmd_custom_list_start = .;
KEEP(*(SORT_BY_NAME("._nrf_modem_at_cmd_custom.*")));
_nrf_modem_at_cmd_custom_list_end = .;
