SECTION_DATA_PROLOGUE(cloud_backends_sections,,)
{
	__cloud_backends_start = .;
	KEEP(*(".cloud_backends"));
	__cloud_backends_end = .;
} GROUP_LINK_IN(ROMABLE_REGION)
