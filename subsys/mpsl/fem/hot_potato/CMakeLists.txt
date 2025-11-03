zephyr_library_sources(fem_psemi.c)
zephyr_library_sources_ifdef(CONFIG_POWER_MAP_MODEL power_model.c)
add_subdirectory_ifdef(CONFIG_POWER_MAP_MODEL models/power_map)

# Set EXTERNAL_FEM_PATH to point to psemi directory for Dragoon MPSL FEM build
# This allows the fem_psemi implementation to be compiled as part of the external FEM library
set(EXTERNAL_FEM_PATH ${CMAKE_CURRENT_SOURCE_DIR}/psemi CACHE PATH "Path to external FEM implementation directory")
