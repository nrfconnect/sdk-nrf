# Make the folder that contains the CA certificate for "test.mosquitto.org" global so that it can
# be located by the MQTT shell backend.
#
# The MQTT shell backend provisions the certificate
# to the native Mbed TLS for nRF7002 DK
zephyr_include_directories_ifdef(CONFIG_SHELL_BACKEND_MQTT_PROVISION_CERTIFICATES certificates)

# Testing purposes
target_compile_options(app PRIVATE "-flto")
