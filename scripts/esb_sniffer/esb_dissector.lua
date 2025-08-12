--
-- Copyright (c) 2025 Nordic Semiconductor ASA
--
-- SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
--

user_link_layer = Proto("ESB", "Enhanced ShockBurst")

LENGTH = ProtoField.uint8("ESB.LENGTH", "Payload length", base.DEC)
PIPE = ProtoField.uint8("ESB.PIPE", "PIPE", base.DEC)
RSSI = ProtoField.uint8("ESB.RSSI", "RSSI", base.DEC)
NOACK = ProtoField.uint8("ESB.NOACK", "NOACK", base.DEC)
PID = ProtoField.uint8("ESB.PID", "PID", base.DEC)

user_link_layer.fields = {LENGTH, PIPE, RSSI, NOACK, PID}

function user_link_layer.dissector(buffer, pinfo, tree)
	pinfo.cols.protocol = user_link_layer.name
	packet_length = buffer:len()

	local subtree = tree:add(user_link_layer, buffer(), "Header")
	-- Length
	local offset = 0
	subtree:add(LENGTH, buffer(offset, 1))

	-- Pipe
	offset = offset + 1
	subtree:add(PIPE, buffer(offset, 1))

	-- Rssi
	offset = offset + 1
	subtree:add(RSSI, buffer(offset, 1))

	-- Noack
	offset = offset + 1
	subtree:add(NOACK, buffer(offset, 1))

	-- Pid
	offset = offset + 1
	subtree:add(PID, buffer(offset, 1))

	-- Data
	offset = offset + 1
	local data_buffer = buffer:range(offset, packet_length-offset):tvb()
	Dissector.get("data"):call(data_buffer, pinfo, tree)
end
