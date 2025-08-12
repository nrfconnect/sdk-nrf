if (NOT wifi-enterprise IN_LIST ${DEFAULT_IMAGE}_SNIPPET)
  set(${DEFAULT_IMAGE}_SNIPPET wifi-enterprise ${${DEFAULT_IMAGE}_SNIPPET} CACHE STRING "" FORCE)
endif()
