. = ALIGN(4);
_ext_apis_start = .;
KEEP(*(.ext_apis))
_ext_apis_size = ABSOLUTE(. - _ext_apis_start);
