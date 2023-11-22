cmake_minimum_required(VERSION 3.20.0)

# Standard Zephyr application boilerplate.
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(smp_svr)

set(smp_srv_dir $ENV{ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr)
target_sources(app PRIVATE ${smp_srv_dir}/src/main.c)
target_sources(app PRIVATE ${smp_srv_dir}/src/bluetooth.c)
