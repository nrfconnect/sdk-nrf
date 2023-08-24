#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import sys
import os

cer_file_name = sys.argv[1]
cer_file = cer_file_name
header_file = os.path.splitext(cer_file_name)[0] + ".h"

key = open(cer_file, "r")
c_file = open(header_file, "w")

c_file.write("static const unsigned char " + os.path.splitext(cer_file_name)[0] + "[] = \n")

for line in key:
    line = line.replace("\n", "")
    c_line = "\t\"" + line + "\\n\"\n"
    c_file.write(c_line)

c_file.write(";\n")
print('Certificate converted to C header file in:', header_file)
key.close()
c_file.close()
