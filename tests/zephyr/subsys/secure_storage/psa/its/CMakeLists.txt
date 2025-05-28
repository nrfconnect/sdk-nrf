cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/subsys/secure_storage/psa/its/src/main.c)

zephyr_sources_ifdef(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_CUSTOM
  ${ZEPHYR_BASE}/tests/subsys/secure_storage/psa/its/src/custom_transform.c)

zephyr_sources_ifdef(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_CUSTOM
  ${ZEPHYR_BASE}/tests/subsys/secure_storage/psa/its/src/custom_store.c)
