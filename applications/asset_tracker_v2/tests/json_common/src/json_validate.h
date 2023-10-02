/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define TEST_VALIDATE_BATTERY_JSON_SCHEMA							\
				"{"								\
					"\"bat\":{"						\
						"\"v\":3600,"					\
						"\"ts\":1563968747123"				\
					"}"							\
				"}"

#define TEST_VALIDATE_GNSS_JSON_SCHEMA								\
				"{"								\
					"\"gnss\":{"						\
						"\"v\":{"					\
							"\"lng\":10,"				\
							"\"lat\":62,"				\
							"\"acc\":24,"				\
							"\"alt\":170,"				\
							"\"spd\":1,"				\
							"\"hdg\":176"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"}"

#define TEST_VALIDATE_ENVIRONMENTAL_JSON_SCHEMA							\
				"{"								\
					"\"env\":{"						\
						"\"v\":{"					\
							"\"temp\":23,"				\
							"\"hum\":50,"				\
							"\"atmp\":101,"				\
							"\"bsec_iaq\":50"			\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"}"

#define TEST_VALIDATE_ENVIRONMENTAL_JSON_SCHEMA_AIR_QUALITY_DISABLED				\
				"{"								\
					"\"env\":{"						\
						"\"v\":{"					\
							"\"temp\":23,"				\
							"\"hum\":50,"				\
							"\"atmp\":101"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"}"

#define TEST_VALIDATE_MODEM_DYNAMIC_JSON_SCHEMA							\
				"{"								\
					"\"roam\":{"						\
						"\"v\":{"					\
							"\"band\":3,"				\
							"\"nw\":\"NB-IoT\","			\
							"\"rsrp\":-8,"				\
							"\"area\":12,"				\
							"\"mccmnc\":24202,"			\
							"\"cell\":33703719,"			\
							"\"ip\":\"10.81.183.99\""		\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"}"

#define TEST_VALIDATE_MODEM_STATIC_JSON_SCHEMA							\
				"{"								\
					"\"dev\":{"						\
						"\"v\":{"					\
							"\"imei\":\"352656106111232\","		\
							"\"iccid\":\"89450421180216211234\","	\
							"\"modV\":\"mfw_nrf9160_1.2.3\","	\
							"\"brdV\":\"nrf9160dk_nrf9160\","	\
							"\"appV\":\"v1.0.0-development\""	\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"}"

#define TEST_VALIDATE_UI_JSON_SCHEMA								\
				"{"								\
					"\"btn\":{"						\
						"\"v\":1,"					\
						"\"ts\":1563968747123"				\
					"}"							\
				"}"

#define TEST_VALIDATE_IMPACT_JSON_SCHEMA							\
				"{"								\
					"\"impact\":{"						\
						"\"v\":300,"					\
						"\"ts\":1563968747123"				\
					"}"							\
				"}"

#define TEST_VALIDATE_NEIGHBOR_CELLS_JSON_SCHEMA						\
				"{"								\
					"\"lte\":{"						\
						"\"mcc\":242,"					\
						"\"mnc\":1,"					\
						"\"cell\":21679716,"				\
						"\"area\":40401,"				\
						"\"earfcn\":6446,"				\
						"\"adv\":80,"					\
						"\"rsrp\":-7,"					\
						"\"rsrq\":28,"					\
						"\"ts\":1563968747123,"				\
						"\"nmr\":["					\
							"{"					\
								"\"earfcn\":262143,"		\
								"\"cell\":501,"			\
								"\"rsrp\":-8,"			\
								"\"rsrq\":25"			\
							"},"					\
							"{"					\
								"\"earfcn\":262265,"		\
								"\"cell\":503,"			\
								"\"rsrp\":-5,"			\
								"\"rsrq\":20"			\
							"}"					\
						"]"						\
					"}"							\
				"}"

#define TEST_VALIDATE_WIFI_AP_JSON_DATA								\
				"{"								\
					"\"wifi\":{"						\
						"\"ts\":1563968747123,"				\
						"\"aps\":["					\
							"\"1300a5a0d29c\","			\
							"\"5c35b5c27b3e\","			\
							"\"7344f6c900cd\","			\
							"\"545e8d443d81\""			\
						"]"						\
					"}"							\
				"}"

#define TEST_VALIDATE_AGNSS_REQUEST_JSON_SCHEMA							\
				"{"								\
					"\"mcc\":242,"						\
					"\"mnc\":1,"						\
					"\"area\":40401,"					\
					"\"cell\":21679716,"					\
					"\"types\":["						\
						"1,"						\
						"2,"						\
						"3,"						\
						"4,"						\
						"5,"						\
						"6,"						\
						"7,"						\
						"8,"						\
						"9"						\
					"]"							\
				"}"

#define TEST_VALIDATE_PGPS_REQUEST_JSON_SCHEMA							\
				"{"								\
					"\"n\":42,"						\
					"\"int\":240,"						\
					"\"day\":15160,"					\
					"\"time\":40655"					\
				"}"

#define TEST_VALIDATE_ARRAY_BATTERY_JSON_SCHEMA							\
				"["								\
					"{"							\
						"\"v\":3600,"					\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"

#define TEST_VALIDATE_ARRAY_GNSS_JSON_SCHEMA							\
				"["								\
					"{"							\
						"\"v\":{"					\
							"\"lng\":10,"				\
							"\"lat\":62,"				\
							"\"acc\":24,"				\
							"\"alt\":170,"				\
							"\"spd\":1,"				\
							"\"hdg\":176"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"

#define TEST_VALIDATE_ARRAY_ENVIRONMENTAL_JSON_SCHEMA						\
				"["								\
					"{"							\
						"\"v\":{"					\
							"\"temp\":23,"				\
							"\"hum\":50,"				\
							"\"atmp\":101,"				\
							"\"bsec_iaq\":55"			\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"

#define TEST_VALIDATE_ARRAY_MODEM_DYNAMIC_JSON_SCHEMA						\
				"["								\
					"{"							\
						"\"v\":{"					\
							"\"band\":20,"				\
							"\"nw\":\"LTE-M\","			\
							"\"rsrp\":-8,"				\
							"\"area\":12,"				\
							"\"mccmnc\":24202,"			\
							"\"cell\":33703719,"			\
							"\"ip\":\"10.81.183.99\""		\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"

#define TEST_VALIDATE_ARRAY_MODEM_STATIC_JSON_SCHEMA						\
				"["								\
					"{"							\
						"\"v\":{"					\
							"\"imei\":\"352656106111232\","		\
							"\"iccid\":\"89450421180216211234\","	\
							"\"modV\":\"mfw_nrf9160_1.2.3\","	\
							"\"brdV\":\"nrf9160dk_nrf9160\","	\
							"\"appV\":\"v1.0.0-development\""	\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"

#define TEST_VALIDATE_ARRAY_UI_JSON_SCHEMA							\
				"["								\
					"{"							\
						"\"v\":1,"					\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"

#define TEST_VALIDATE_ARRAY_IMPACT_JSON_SCHEMA							\
				"["								\
					"{"							\
						"\"v\":300,"					\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"

#define TEST_VALIDATE_CONFIGURATION_JSON_SCHEMA							\
				"{"								\
					"\"cfg\":{"						\
						"\"act\":false,"				\
						"\"loct\":60,"					\
						"\"actwt\":120,"				\
						"\"mvres\":120,"				\
						"\"mvt\":3600,"					\
						"\"accath\":10,"				\
						"\"accith\":5,"					\
						"\"accito\":80,"				\
						"\"nod\":["					\
							"\"gnss\","				\
							"\"ncell\""				\
						"]"						\
					"}"							\
				"}"

#define TEST_VALIDATE_BATCH_JSON_SCHEMA								\
			"{"									\
				"\"bat\":["							\
					"{"							\
						"\"v\":3600,"					\
						"\"ts\":1563968747123"				\
					"},"							\
					"{"							\
						"\"v\":3600,"					\
						"\"ts\":1563968747123"				\
					"}"							\
				"],"								\
				"\"btn\":["							\
					"{"							\
						"\"v\":1,"					\
						"\"ts\":1563968747123"				\
					"},"							\
					"{"							\
						"\"v\":1,"					\
						"\"ts\":1563968747123"				\
					"}"							\
				"],"								\
				"\"impact\":["							\
					"{"							\
						"\"v\":300,"					\
						"\"ts\":1563968747123"				\
					"},"							\
					"{"							\
						"\"v\":300,"					\
						"\"ts\":1563968747123"				\
					"}"							\
				"],"								\
				"\"gnss\":["							\
					"{"							\
						"\"v\":{"					\
							"\"lng\":10,"				\
							"\"lat\":62,"				\
							"\"acc\":24,"				\
							"\"alt\":170,"				\
							"\"spd\":1,"				\
							"\"hdg\":176"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"},"							\
					"{"							\
						"\"v\":{"					\
							"\"lng\":10,"				\
							"\"lat\":62,"				\
							"\"acc\":24,"				\
							"\"alt\":170,"				\
							"\"spd\":1,"				\
							"\"hdg\":176"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"],"								\
				"\"env\":["							\
					"{"							\
						"\"v\":{"					\
							"\"temp\":23,"				\
							"\"hum\":50,"				\
							"\"atmp\":80,"				\
							"\"bsec_iaq\":50"			\
						"},"						\
						"\"ts\":1563968747123"				\
					"},"							\
					"{"							\
						"\"v\":{"					\
							"\"temp\":23,"				\
							"\"hum\":50,"				\
							"\"atmp\":101,"				\
							"\"bsec_iaq\":55"			\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"],"								\
				"\"roam\":["							\
					"{"							\
						"\"v\":{"					\
							"\"band\":3,"				\
							"\"nw\":\"NB-IoT\","			\
							"\"rsrp\":-8,"				\
							"\"area\":12,"				\
							"\"mccmnc\":24202,"			\
							"\"cell\":33703719,"			\
							"\"ip\":\"10.81.183.99\""		\
						"},"						\
						"\"ts\":1563968747123"				\
					"},"							\
					"{"							\
						"\"v\":{"					\
							"\"band\":20,"				\
							"\"nw\":\"LTE-M\","			\
							"\"rsrp\":-5,"				\
							"\"area\":12,"				\
							"\"mccmnc\":24202,"			\
							"\"cell\":33703719,"			\
							"\"ip\":\"10.81.183.99\""		\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"],"								\
				"\"dev\":["							\
					"{"							\
						"\"v\":{"					\
							"\"imei\":\"352656106111232\","		\
							"\"iccid\":\"89450421180216211234\","	\
							"\"modV\":\"mfw_nrf9160_1.2.3\","	\
							"\"brdV\":\"nrf9160dk_nrf9160\","	\
							"\"appV\":\"v1.0.0-development\""	\
						"},"						\
						"\"ts\":1563968747123"				\
					"},"							\
					"{"							\
						"\"v\":{"					\
							"\"imei\":\"352656106111232\","		\
							"\"iccid\":\"89450421180216211234\","	\
							"\"modV\":\"mfw_nrf9160_1.2.3\","	\
							"\"brdV\":\"nrf9160dk_nrf9160\","	\
							"\"appV\":\"v1.0.0-development\""	\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"								\
			"}"
