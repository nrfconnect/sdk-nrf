/* CFUN callbacks */
. = ALIGN(4);
_lte_lc_cfun_cb_list_start = .;
KEEP(*(SORT_BY_NAME("._lte_lc_cfun_cb.*")));
_lte_lc_cfun_cb_list_end = .;
