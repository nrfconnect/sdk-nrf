This sample extends the same-named Zephyr sample to verify it with Nordic development kits.

Source code and basic configuration files can be found in the corresponding folder structure in zephyr/samples/net/sockets/coap_server.

Building
********

This directory has no prj.conf of its own, so build it against one of the test configurations in sample.yaml, for example:

   west build -p -b nrf7120dk/nrf7120/cpuapp -T nrf.extended.sample.net.sockets.coap_server.wifi .

Otherwise you need to add extra argument -DCONF_FILE='${ZEPHYR_BASE}/samples/net/sockets/coap_server/prj.conf' to the build command, for example:

   west build -p -b nrf7120dk/nrf7120/cpuapp -- -DCONF_FILE='${ZEPHYR_BASE}/samples/net/sockets/coap_server/prj.conf'
