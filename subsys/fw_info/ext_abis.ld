. = ALIGN(4);
_ext_abis_start = .;
KEEP(*(.ext_abis))
_ext_abis_size = ABSOLUTE((. - _ext_abis_start) / 4);
