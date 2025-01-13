/* DL transports */
. = ALIGN(4);
_dl_transport_entry_list_start = .;
KEEP(*(SORT_BY_NAME("._dl_transport_entry.*")));
_dl_transport_entry_list_end = .;

ASSERT(_dl_transport_entry_list_end != _dl_transport_entry_list_start,
       "No transport enabled for the downloader library. See downloader library Kconfig options.");
