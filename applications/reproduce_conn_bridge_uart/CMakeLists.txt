cmake_minimum_required(VERSION 3.20.0)

# Set your project name here
set(APP_NAME my_uart_logging_app)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(reproduce_conn_bridge_uart)

# Add your source files here
target_sources(app PRIVATE src/main.c)


#cmake_minimum_required(VERSION 3.20.0)

#find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
#project(reproduce)
#zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})

# NORDIC SDK APP START
#target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

# Application sources
#add_subdirectory(src)
