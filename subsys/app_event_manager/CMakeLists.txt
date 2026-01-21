#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
zephyr_sources(app_event_manager.c)
zephyr_sources_ifdef(CONFIG_APP_EVENT_MANAGER_SHELL app_event_manager_shell.c)

zephyr_linker_sources(SECTIONS aem.ld)
zephyr_iterable_section(NAME event_type KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_iterable_section(NAME event_listener KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_iterable_section(NAME app_event_manager_postinit_hook KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_iterable_section(NAME event_submit_hook KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_iterable_section(NAME event_preprocess_hook KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_iterable_section(NAME event_postprocess_hook KVMA RAM_REGION GROUP RODATA_REGION)

zephyr_linker_section(NAME event_subscribers_all KVMA RAM_REGION GROUP RODATA_REGION NOINPUT)
zephyr_linker_section_configure(SECTION event_subscribers_all
                                INPUT "event_subscribers_*"
                                SYMBOLS "__start_event_subscribers_all" "__stop_event_subscribers_all"
                                KEEP SORT NAME)
