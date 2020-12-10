# The macro is used by lib/edge_impulse/CMakeLists.ei.template file while building the zipped Edge
# Impulse library in the NCS. The CMakeLists.ei.template uses this macro to find source files of the
# machine learning model.

MACRO(RECURSIVE_FIND_FILE return_list dir pattern)
    FILE(GLOB_RECURSE new_list "${dir}/${pattern}")
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        SET(dir_list ${dir_list} ${file_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()
