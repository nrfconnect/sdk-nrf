. = CONFIG_FW_INFO_OFFSET;
_fw_info_start = .;
KEEP(*(SORT_BY_NAME(.firmware_info*)))
_fw_info_size = ABSOLUTE(. - _fw_info_start);
