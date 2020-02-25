# Included by zephyr/cmake/extensions.cmake

# Usage:
#   add_partition_manager_config(pm.yml)
#
# Will add all configurations defined in pm.yml to the global list of partition
# manager configurations.
function(add_partition_manager_config config_file)
  get_filename_component(pm_path ${config_file} REALPATH)
  set_property(
    GLOBAL APPEND PROPERTY
    PM_SUBSYS
    ${pm_path}
    )
endfunction()
