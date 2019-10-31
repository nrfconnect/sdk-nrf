. = ALIGN(4);
_fw_info_images_start = .;
KEEP(*(.fw_info_images))
_fw_info_images_size = ABSOLUTE((. - _fw_info_images_start) / 4);

_ext_apis_start = .;
KEEP(*(.ext_apis))
_ext_apis_size = ABSOLUTE(. - _ext_apis_start);

_ext_apis_req_start = .;
KEEP(*(.ext_apis_req))
_ext_apis_req_size = ABSOLUTE(. - _ext_apis_req_start);
