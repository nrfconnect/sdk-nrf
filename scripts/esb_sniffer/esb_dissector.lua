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

	local len   = buffer(0, 1)
	local pipe  = buffer(1, 1)
	local rssi  = buffer(2, 1)
	local noack = buffer(3, 1)
	local pid   = buffer(4, 1)
	local data  = buffer:range(5, packet_length-5):tvb()
	local subtree = tree:add(user_link_layer, buffer(), "Header")

	-- Length
	subtree:add(LENGTH, len)
	if len:uint() > 252
	then
		subtree:add_expert_info(PI_MALFORMED, PI_WARN, "Wrong data length")
	end

	-- Pipe
	subtree:add(PIPE, pipe)
	if pipe:uint() > 7
	then
		subtree:add_expert_info(PI_MALFORMED, PI_WARN, "Wrong PIPE number")
	end

	-- Rssi
	subtree:add(RSSI, rssi)

	-- Noack
	subtree:add(NOACK, noack)
	if noack:uint() > 1
	then
		subtree:add_expert_info(PI_MALFORMED, PI_WARN, "Wrong NOACK value")
	end

	-- Pid
	subtree:add(PID, pid)
	if pid:uint() > 3
	then
		subtree:add_expert_info(PI_MALFORMED, PI_WARN, "Wrong PID vaue")
	end

	-- Data
	Dissector.get("data"):call(data, pinfo, tree)
end
