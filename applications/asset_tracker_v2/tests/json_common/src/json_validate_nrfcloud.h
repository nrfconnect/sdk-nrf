/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define TEST_VALIDATE_BATTERY_JSON_SCHEMA\
				"{"\
					"\"batteryVoltage\":{"\
						"\"data\":3600,"\
						"\"ts\":1563968747123"\
					"}"\
				"}"

#define TEST_VALIDATE_GNSS_JSON_SCHEMA\
				"{"\
					"\"gnss\":{"\
						"\"data\":{"\
							"\"lng\":10,"\
							"\"lat\":62,"\
							"\"acc\":24,"\
							"\"alt\":170,"\
							"\"spd\":1,"\
							"\"hdg\":176"\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"}"

#define TEST_VALIDATE_ENVIRONMENTAL_JSON_SCHEMA\
				"{"\
					"\"env\":{"\
						"\"data\":{"\
							"\"temp\":23,"\
							"\"hum\":50,"\
							"\"atmp\":101,"\
							"\"bsec_iaq\":50"\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"}"

#define TEST_VALIDATE_ENVIRONMENTAL_JSON_SCHEMA_AIR_QUALITY_DISABLED\
				"{"\
					"\"env\":{"\
						"\"data\":{"\
							"\"temp\":23,"\
							"\"hum\":50,"\
							"\"atmp\":101"\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"}"

#define TEST_VALIDATE_MODEM_DYNAMIC_JSON_SCHEMA\
				"{"\
					"\"networkInfo\":{"\
						"\"data\":{"\
							"\"currentBand\":3,"\
							"\"networkMode\":\"NB-IoT\","\
							"\"rsrp\":-8,"\
							"\"areaCode\":12,"\
							"\"mccmnc\":24202,"\
							"\"cellID\":33703719,"\
							"\"ipAddress\":\"10.81.183.99\""\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"}"

#define TEST_VALIDATE_MODEM_STATIC_JSON_SCHEMA\
				"{"\
					"\"deviceInfo\":{"\
						"\"data\":{"\
							"\"imei\":\"352656106111232\","\
							"\"iccid\":\"89450421180216211234\","\
							"\"modemFirmware\":\"mfw_nrf9160_1.2.3\","\
							"\"board\":\"nrf9160dk_nrf9160\","\
							"\"appVersion\":\"v1.0.0-development\""\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"}"

#define TEST_VALIDATE_UI_JSON_SCHEMA\
				"{"\
					"\"btn\":{"\
						"\"data\":1,"\
						"\"ts\":1563968747123"\
					"}"\
				"}"

#define TEST_VALIDATE_NEIGHBOR_CELLS_JSON_SCHEMA\
				"{"\
					"\"mcc\":242,"\
					"\"mnc\":1,"\
					"\"eci\":21679716,"\
					"\"tac\":40401,"\
					"\"earfcn\":6446,"\
					"\"adv\":80,"\
					"\"rsrp\":-7,"\
					"\"rsrq\":28,"\
					"\"ts\":1563968747123,"\
					"\"nmr\":["\
						"{"\
							"\"earfcn\":262143,"\
							"\"pci\":501,"\
							"\"rsrp\":-8,"\
							"\"rsrq\":25"\
						"},"\
						"{"\
							"\"earfcn\":262265,"\
							"\"pci\":503,"\
							"\"rsrp\":-5,"\
							"\"rsrq\":20"\
						"}"\
					"]"\
				"}"

#define TEST_VALIDATE_AGPS_REQUEST_JSON_SCHEMA\
				"{"\
					"\"mcc\":242,"\
					"\"mnc\":1,"\
					"\"area\":40401,"\
					"\"cell\":21679716,"\
					"\"types\":["\
						"1,"\
						"2,"\
						"3,"\
						"4,"\
						"6,"\
						"7,"\
						"8,"\
						"9"\
					"]"\
				"}"

#define TEST_VALIDATE_PGPS_REQUEST_JSON_SCHEMA\
				"{"\
					"\"n\":42,"\
					"\"int\":240,"\
					"\"day\":15160,"\
					"\"time\":40655"\
				"}"

#define TEST_VALIDATE_ACCELEROMETER_JSON_SCHEMA\
				"{"\
					"\"acc\":{"\
						"\"data\":{"\
							"\"x\":1,"\
							"\"y\":2,"\
							"\"z\":3"\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"}"

#define TEST_VALIDATE_ARRAY_BATTERY_JSON_SCHEMA\
				"["\
					"{"\
						"\"data\":3600,"\
						"\"ts\":1563968747123"\
					"}"\
				"]"

#define TEST_VALIDATE_ARRAY_GNSS_JSON_SCHEMA\
				"["\
					"{"\
						"\"data\":{"\
							"\"lng\":10,"\
							"\"lat\":62,"\
							"\"acc\":24,"\
							"\"alt\":170,"\
							"\"spd\":1,"\
							"\"hdg\":176"\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"]"

#define TEST_VALIDATE_ARRAY_ENVIRONMENTAL_JSON_SCHEMA\
				"["\
					"{"\
						"\"data\":{"\
							"\"temp\":23,"\
							"\"hum\":50,"\
							"\"atmp\":101,"\
							"\"bsec_iaq\":55"\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"]"

#define TEST_VALIDATE_ARRAY_MODEM_DYNAMIC_JSON_SCHEMA\
				"["\
					"{"\
						"\"data\":{"\
							"\"currentBand\":20,"\
							"\"networkMode\":\"LTE-M\","\
							"\"rsrp\":-8,"\
							"\"areaCode\":12,"\
							"\"mccmnc\":24202,"\
							"\"cellID\":33703719,"\
							"\"ipAddress\":\"10.81.183.99\""\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"]"

#define TEST_VALIDATE_ARRAY_MODEM_STATIC_JSON_SCHEMA\
				"["\
					"{"\
						"\"data\":{"\
							"\"imei\":\"352656106111232\","\
							"\"iccid\":\"89450421180216211234\","\
							"\"modemFirmware\":\"mfw_nrf9160_1.2.3\","\
							"\"board\":\"nrf9160dk_nrf9160\","\
							"\"appVersion\":\"v1.0.0-development\""\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"]"

#define TEST_VALIDATE_ARRAY_UI_JSON_SCHEMA\
				"["\
					"{"\
						"\"data\":1,"\
						"\"ts\":1563968747123"\
					"}"\
				"]"

#define TEST_VALIDATE_ARRAY_ACCELEROMETER_JSON_SCHEMA\
				"["\
					"{"\
						"\"data\":{"\
							"\"x\":1,"\
							"\"y\":2,"\
							"\"z\":3"\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"]"

#define TEST_VALIDATE_CONFIGURATION_JSON_SCHEMA\
				"{"\
					"\"config\":{"\
						"\"activeMode\":false,"\
						"\"gnssTimeout\":60,"\
						"\"activeWaitTime\":120,"\
						"\"movementResolution\":120,"\
						"\"movementTimeout\":3600,"\
						"\"movementThreshold\":2,"\
						"\"nod\":["\
							"\"gnss\","\
							"\"ncell\""\
						"]"\
					"}"\
				"}"

#define TEST_VALIDATE_BATCH_JSON_SCHEMA\
			"{"\
				"\"batteryVoltage\":["\
					"{"\
						"\"data\":3600,"\
						"\"ts\":1563968747123"\
					"},"\
					"{"\
						"\"data\":3600,"\
						"\"ts\":1563968747123"\
					"}"\
				"],"\
				"\"btn\":["\
					"{"\
						"\"data\":1,"\
						"\"ts\":1563968747123"\
					"},"\
					"{"\
						"\"data\":1,"\
						"\"ts\":1563968747123"\
					"}"\
				"],"\
				"\"gnss\":["\
					"{"\
						"\"data\":{"\
							"\"lng\":10,"\
							"\"lat\":62,"\
							"\"acc\":24,"\
							"\"alt\":170,"\
							"\"spd\":1,"\
							"\"hdg\":176"\
						"},"\
						"\"ts\":1563968747123"\
					"},"\
					"{"\
						"\"data\":{"\
							"\"lng\":10,"\
							"\"lat\":62,"\
							"\"acc\":24,"\
							"\"alt\":170,"\
							"\"spd\":1,"\
							"\"hdg\":176"\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"],"\
				"\"env\":["\
					"{"\
						"\"data\":{"\
							"\"temp\":23,"\
							"\"hum\":50,"\
							"\"atmp\":80,"\
							"\"bsec_iaq\":50"\
						"},"\
						"\"ts\":1563968747123"\
					"},"\
					"{"\
						"\"data\":{"\
							"\"temp\":23,"\
							"\"hum\":50,"\
							"\"atmp\":101,"\
							"\"bsec_iaq\":55"\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"],"\
				"\"acc\":["\
					"{"\
						"\"data\":{"\
							"\"x\":1,"\
							"\"y\":2,"\
							"\"z\":3"\
						"},"\
						"\"ts\":1563968747123"\
					"},"\
					"{"\
						"\"data\":{"\
							"\"x\":1,"\
							"\"y\":2,"\
							"\"z\":3"\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"],"\
				"\"networkInfo\":["\
					"{"\
						"\"data\":{"\
							"\"currentBand\":3,"\
							"\"networkMode\":\"NB-IoT\","\
							"\"rsrp\":-8,"\
							"\"areaCode\":12,"\
							"\"mccmnc\":24202,"\
							"\"cellID\":33703719,"\
							"\"ipAddress\":\"10.81.183.99\""\
						"},"\
						"\"ts\":1563968747123"\
					"},"\
					"{"\
						"\"data\":{"\
							"\"currentBand\":20,"\
							"\"networkMode\":\"LTE-M\","\
							"\"rsrp\":-5,"\
							"\"areaCode\":12,"\
							"\"mccmnc\":24202,"\
							"\"cellID\":33703719,"\
							"\"ipAddress\":\"10.81.183.99\""\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"],"\
				"\"deviceInfo\":["\
					"{"\
						"\"data\":{"\
							"\"imei\":\"352656106111232\","\
							"\"iccid\":\"89450421180216211234\","\
							"\"modemFirmware\":\"mfw_nrf9160_1.2.3\","\
							"\"board\":\"nrf9160dk_nrf9160\","\
							"\"appVersion\":\"v1.0.0-development\""\
						"},"\
						"\"ts\":1563968747123"\
					"},"\
					"{"\
						"\"data\":{"\
							"\"imei\":\"352656106111232\","\
							"\"iccid\":\"89450421180216211234\","\
							"\"modemFirmware\":\"mfw_nrf9160_1.2.3\","\
							"\"board\":\"nrf9160dk_nrf9160\","\
							"\"appVersion\":\"v1.0.0-development\""\
						"},"\
						"\"ts\":1563968747123"\
					"}"\
				"]"\
			"}"
