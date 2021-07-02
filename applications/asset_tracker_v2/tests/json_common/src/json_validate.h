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

#define TEST_VALIDATE_GPS_JSON_SCHEMA								\
				"{"								\
					"\"gps\":{"						\
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
							"\"hum\":50"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"}"

#define TEST_VALIDATE_MODEM_DYNAMIC_JSON_SCHEMA							\
				"{"								\
					"\"roam\":{"						\
						"\"v\":{"					\
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
							"\"band\":3,"				\
							"\"nw\":\"NB-IoT GPS\","		\
							"\"iccid\":\"89450421180216216095\","	\
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

#define TEST_VALIDATE_NEIGHBOR_CELLS_JSON_SCHEMA						\
				"{"								\
					"\"mcc\":242,"						\
					"\"mnc\":1,"						\
					"\"cell\":21679716,"					\
					"\"area\":40401,"					\
					"\"earfcn\":6446,"					\
					"\"adv\":80,"						\
					"\"rsrp\":-7,"						\
					"\"rsrq\":28,"						\
					"\"ts\":1563968747123,"					\
					"\"nmr\":["						\
						"{"						\
							"\"earfcn\":262143,"			\
							"\"cell\":501,"				\
							"\"rsrp\":-8,"				\
							"\"rsrq\":25"				\
						"},"						\
						"{"						\
							"\"earfcn\":262265,"			\
							"\"cell\":503,"				\
							"\"rsrp\":-5,"				\
							"\"rsrq\":20"				\
						"}"						\
					"]"							\
				"}"

#define TEST_VALIDATE_ACCELEROMETER_JSON_SCHEMA							\
				"{"								\
					"\"acc\":{"						\
						"\"v\":{"					\
							"\"x\":1,"				\
							"\"y\":2,"				\
							"\"z\":3"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"}"

#define TEST_VALIDATE_ARRAY_BATTERY_JSON_SCHEMA							\
				"["								\
					"{"							\
						"\"v\":3600,"					\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"

#define TEST_VALIDATE_ARRAY_GPS_JSON_SCHEMA							\
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
							"\"hum\":50"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"

#define TEST_VALIDATE_ARRAY_MODEM_DYNAMIC_JSON_SCHEMA						\
				"["								\
					"{"							\
						"\"v\":{"					\
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
							"\"band\":3,"				\
							"\"nw\":\"NB-IoT GPS\","		\
							"\"iccid\":\"89450421180216216095\","	\
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

#define TEST_VALIDATE_ARRAY_ACCELEROMETER_JSON_SCHEMA						\
				"["								\
					"{"							\
						"\"v\":{"					\
							"\"x\":1,"				\
							"\"y\":2,"				\
							"\"z\":3"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"

#define TEST_VALIDATE_CONFIGURATION_JSON_SCHEMA							\
				"{"								\
					"\"cfg\":{"						\
						"\"act\":false,"				\
						"\"gpst\":60,"					\
						"\"actwt\":120,"				\
						"\"mvres\":120,"				\
						"\"mvt\":3600,"					\
						"\"acct\":2,"					\
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
				"\"gps\":["							\
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
							"\"hum\":50"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"},"							\
					"{"							\
						"\"v\":{"					\
							"\"temp\":23,"				\
							"\"hum\":50"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"],"								\
				"\"acc\":["							\
					"{"							\
						"\"v\":{"					\
							"\"x\":1,"				\
							"\"y\":2,"				\
							"\"z\":3"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"},"							\
					"{"							\
						"\"v\":{"					\
							"\"x\":1,"				\
							"\"y\":2,"				\
							"\"z\":3"				\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"],"								\
				"\"roam\":["							\
					"{"							\
						"\"v\":{"					\
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
							"\"band\":3,"				\
							"\"nw\":\"NB-IoT GPS\","		\
							"\"iccid\":\"89450421180216216095\","	\
							"\"modV\":\"mfw_nrf9160_1.2.3\","	\
							"\"brdV\":\"nrf9160dk_nrf9160\","	\
							"\"appV\":\"v1.0.0-development\""	\
						"},"						\
						"\"ts\":1563968747123"				\
					"},"							\
					"{"							\
						"\"v\":{"					\
							"\"band\":3,"				\
							"\"nw\":\"NB-IoT GPS\","		\
							"\"iccid\":\"89450421180216216095\","	\
							"\"modV\":\"mfw_nrf9160_1.2.3\","	\
							"\"brdV\":\"nrf9160dk_nrf9160\","	\
							"\"appV\":\"v1.0.0-development\""	\
						"},"						\
						"\"ts\":1563968747123"				\
					"}"							\
				"]"								\
			"}"
