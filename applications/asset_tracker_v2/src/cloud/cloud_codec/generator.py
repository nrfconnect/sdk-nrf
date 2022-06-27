#!/usr/bin/env python3
import os

output_dir = os.path.dirname(os.path.realpath(__file__))

# The cloud codec buffers are defined in following format:
# (name, CONFIG_DATA_x_STORE, type, length)
# The second parameter is the name of a KConfig option that is checked prior to putting new elements into the buffer.
buffers = [
    ("sensor", "CONFIG_DATA_SENSOR_BUFFER_STORE", "struct cloud_data_sensors", "CONFIG_DATA_SENSOR_BUFFER_COUNT"),
    ("ui", "CONFIG_DATA_UI_BUFFER_STORE", "struct cloud_data_ui", "CONFIG_DATA_UI_BUFFER_COUNT"),
    ("accel", "CONFIG_DATA_ACCELEROMETER_BUFFER_STORE", "struct cloud_data_accelerometer", "CONFIG_DATA_ACCELEROMETER_BUFFER_COUNT"),
    ("bat", "CONFIG_DATA_BATTERY_BUFFER_STORE", "struct cloud_data_battery", "CONFIG_DATA_BATTERY_BUFFER_COUNT"),
    ("gnss", "CONFIG_DATA_GNSS_BUFFER_STORE", "struct cloud_data_gnss", "CONFIG_DATA_GNSS_BUFFER_COUNT"),
    ("modem_dyn", "CONFIG_DATA_MODEM_BUFFER_STORE", "struct cloud_data_modem_dynamic", "CONFIG_DATA_MODEM_DYNAMIC_BUFFER_COUNT"),
    ("modem_stat", "CONFIG_DATA_MODEM_STAT_BUFFER_STORE", "struct cloud_data_modem_static", "1"),
]

cloud_codec_populate_declaration = \
    """
/** @brief Add an element to the {0} cloud buffer
 *
 * @param[in] new_data data element to add
 */
void cloud_codec_populate_{0}_buffer({2}* new_data);
"""
cloud_codec_populate_definition = \
    """
void cloud_codec_populate_{0}_buffer({2}* new_data) {{
    if (!IS_ENABLED(CONFIG_DATA_{1}_BUFFER_STORE)) {{
		return;
	}}
	if (!new_data->queued) {{
		return;
	}}
	head_{0}_buf += 1;
	/* Go to start of buffer if end is reached. */
	if (head_{0}_buf == {3}) {{
		head_{0}_buf = 0;
	}}
	{0}_buf[head_{0}_buf] = *new_data;
	LOG_DBG("Entry: %d of %d in %s buffer filled",
		head_{0}_buf, {3} - 1, "{0}");
}}
"""
cloud_codec_buffer_definition = "static {2} {0}_buf[{3}];\n"
cloud_codec_buffer_head_definition = "static int head_{0}_buf;\n"

cloud_codec_peek_declaration = \
    """
/* Retrieve the most recent element from the {0} buffer */
{2} *cloud_codec_peek_{0}_buffer();
"""

cloud_codec_peek_definition = \
    """
{2} *cloud_codec_peek_{0}_buffer() {{
    return &{0}_buf[head_{0}_buf];
}}
"""

cloud_codec_list_buffers_declaration = \
    """
/* Retrieve a list of cloud buffers and their lengths.*/
struct cloud_codec_buffers cloud_codec_list_buffers();
"""

cloud_codec_list_buffer_heads_declaration = \
    """
/* Retrieve a list of the most current elements of all cloud buffers.*/
struct cloud_codec_data_to_encode cloud_codec_list_buffer_heads();
"""

license_header = \
    """\
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
"""

cloud_buffers_h_header = \
    """
#ifndef GENERATED_CLOUD_BUFFERS_H__
#define GENERATED_CLOUD_BUFFERS_H__

#include <stdio.h>
#include "cloud_buffer_types.h"

/**@file
 *
 * @defgroup generated_cloud_buffers Cloud codec buffers
 * @brief    Module that contains ringbuffers for cloud data
 *
 * All data received by the Data module is stored in ringbuffers.
 * Upon a LTE connection loss the device will keep sampling/storing data in
 * the buffers, and empty the buffers in batches upon a reconnect.
 *
 * This module is auto-generated using the generator.py script.
 * If you wish to add another cloud buffer, add the data type to
 * cloud_buffer_types.h and add a line to the "buffers" list inside
 * generator.py.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif
"""

cloud_buffers_h_footer = \
    """
#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* GENERATED_CLOUD_BUFFERS_H__ */
"""

cloud_buffers_c_header = \
"""
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "generated_cloud_buffers.h"
LOG_MODULE_REGISTER(cloud_codec_ringbuffer, CONFIG_CLOUD_CODEC_LOG_LEVEL);
"""


def generate_cloud_codec_data_to_encode(buffers: list) -> str:
    result = "/* This struct holds a pointer of each cloud buffer data type. */\n"
    result += "struct cloud_codec_data_to_encode {\n"
    for b in buffers:
        result += "    {2} *{0};\n".format(*b)
    result += "};\n"
    return result


def generate_cloud_codec_buffers(buffers: list) -> str:
    result = "/** This struct holds a pointer to each cloud buffer\n" + \
             " *  and their respective length.\n" + \
             " */\n"
    result += "struct cloud_codec_buffers {\n"
    for b in buffers:
        result += "    {2} *{0}_buf;\n".format(*b)
        result += "    size_t {0}_buf_count;\n".format(*b)
    result += "};\n"
    return result


def generate_cloud_codec_list_buffers(buffers: list) -> str:
    result = "struct cloud_codec_buffers cloud_codec_list_buffers() {\n" + \
             "    struct cloud_codec_buffers result = {\n"
    for b in buffers:
        result += "        .{0}_buf = {0}_buf,\n".format(*b)
        result += "        .{0}_buf_count = ARRAY_SIZE({0}_buf),\n".format(*b)
    result += "    };\n"
    result += "    return result;\n"
    result += "}\n"
    return result


def generate_cloud_codec_list_buffer_heads(buffers: list) -> str:
    result = "struct cloud_codec_data_to_encode cloud_codec_list_buffer_heads() {\n" + \
             "    struct cloud_codec_data_to_encode result = {\n"
    for b in buffers:
        result += "        .{0} = &{0}_buf[head_{0}_buf],\n".format(*b)
    result += "    };\n"
    result += "    return result;\n"
    result += "}\n"
    return result


if __name__ == "__main__":
    with open(os.path.join(output_dir, "generated_cloud_buffers.h"), "w") as f:
        f.write(license_header)
        f.write(cloud_buffers_h_header)
        for b in buffers:
            f.write(cloud_codec_peek_declaration.format(*b))
        f.write("\n")
        for b in buffers:
            f.write(cloud_codec_populate_declaration.format(*b))
        f.write("\n")
        f.write(generate_cloud_codec_data_to_encode(buffers))
        f.write("\n")
        f.write(generate_cloud_codec_buffers(buffers))
        f.write(cloud_codec_list_buffers_declaration)
        f.write(cloud_codec_list_buffer_heads_declaration)
        f.write(cloud_buffers_h_footer)

    with open(os.path.join(output_dir, "generated_cloud_buffers.c"), "w") as f:
        f.write(license_header)
        f.write(cloud_buffers_c_header)
        f.write("\n")
        for b in buffers:
            f.write(cloud_codec_buffer_definition.format(*b))
        for b in buffers:
            f.write(cloud_codec_buffer_head_definition.format(*b))
        for b in buffers:
            f.write(cloud_codec_peek_definition.format(*b))
        for b in buffers:
            f.write(cloud_codec_populate_definition.format(*b))
        f.write("\n")
        f.write(generate_cloud_codec_list_buffers(buffers))
        f.write("\n")
        f.write(generate_cloud_codec_list_buffer_heads(buffers))
