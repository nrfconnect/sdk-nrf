#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# The Gadgets Profile relies on the nanopb generator tool, which must be
# available at the configuration stage.
# This tool is used to compile the alexa .proto files at build time.
# Output from the .proto compilation is a set of .c and .h files.

# Note: as there doesn't seem to be an easy way of checking the version of the nanopb generator (protoc-gen-nanopb binary)
# there is the possibility of a version mismatch between the generator and the nanopb source files that is used in the firmware sample.
# If there is a mismatch, there will be a compilation error in the build step.
# The easiest way to fix such an error is to download the appropriate nanopb generator .zip and extract it to this sample folder.
# (https://jpa.kapsi.fi/nanopb/download/)


# Check system for protoc and nanopb binaries presence, needed to build the gadget profile files.
# Prefer local variable to make version matching easier
find_program(PROTOC_BIN
    protoc
    HINTS
    "${CMAKE_SOURCE_DIR}/*/generator-bin/"
    "${ZEPHYR_BASE}/../*/generator-bin/"
    "${ZEPHYR_BASE}/../tools/*/generator-bin/"
)
find_program(NANOPB_BIN
    protoc-gen-nanopb
    HINTS
    "${CMAKE_SOURCE_DIR}/*/generator-bin/"
    "${ZEPHYR_BASE}/../*/generator-bin/"
    "${ZEPHYR_BASE}/../tools/*/generator-bin/"
)

if (((NOT NANOPB_BIN) OR (NOT PROTOC_BIN)) AND CONFIG_BT_ALEXA_GADGETS_PROFILE)
    message(STATUS "nanopb not found.")
    message(STATUS "Please download platform-specific nanopb from here https://jpa.kapsi.fi/nanopb/download/")
    message(STATUS "Extract the files to `${CMAKE_SOURCE_DIR}`, or add `<path-to>/nanopb-x.y.z-<platform>/generator-bin` to system PATH.")
    message(STATUS "Alternatively, specify install location using: `-DCMAKE_PROGRAM_PATH=<path-to>/nanopb-x.y.z-<platform>/generator-bin` when running CMake.")
    message(FATAL_ERROR "nanopb required by this project.")
endif()

# Locate alexa-embedded and nanopb in local ncs tree
if(ALEXA_EMBEDDED_PATH)
  # Do nothing, just use the provided path
elseif(WEST)
  ## Use Python and west to find the alexa-embedded path
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c
    "from west.manifest import Manifest; manifest = Manifest.from_file(); print(manifest.get_projects(['Alexa-Gadgets-Embedded-Sample-Code'])[0].posixpath)"
    OUTPUT_VARIABLE
    west_project_output
    RESULT_VARIABLE
    result
  )
  string(REGEX REPLACE "[\r\n]+" "" ALEXA_EMBEDDED_PATH "${west_project_output}")
  if(${result})
    message(FATAL_ERROR "Failed to find alexa-embedded, cannot build sample")
  endif()
else()
  message(FATAL_ERROR "west not installed, please provide ALEXA_EMBEDDED_PATH to CMake to build this sample")
endif()

if(NANOPB_PATH)
  # Do nothing, just use the provided path
elseif(WEST)
  ## Use Python and west to find the nanopb path
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c
     "from west.manifest import Manifest; manifest = Manifest.from_file(); print(manifest.get_projects(['nanopb'])[0].posixpath)"
    OUTPUT_VARIABLE
    west_project_output
    RESULT_VARIABLE
    result
  )
  string(REGEX REPLACE "[\r\n]+" "" NANOPB_PATH "${west_project_output}")
  if(${result})
    message(FATAL_ERROR "Failed to find nanopb source files, cannot build sample")
  endif()
else()
  message(FATAL_ERROR "west not installed, please provide NANOPB_PATH to CMake to build this sample")
endif()

# Protobuf generation setup

set(PROTO_COMPILATION_DIR ${CMAKE_BINARY_DIR}/generated/alexa/protobuf)

# Populate lists of .proto files that should be compiled at build time
set(BASE_PROTO_FILES
    ${ALEXA_EMBEDDED_PATH}/ConnectionHelpers/BLE/Proto/accessories.proto
    ${ALEXA_EMBEDDED_PATH}/ConnectionHelpers/BLE/Proto/common.proto
    ${ALEXA_EMBEDDED_PATH}/ConnectionHelpers/BLE/Proto/device.proto
    ${ALEXA_EMBEDDED_PATH}/ConnectionHelpers/BLE/Proto/firmware.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/parser/directiveParser.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/common/directiveHeader.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/parser/eventParser.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/common/eventHeader.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alexa.Discovery/Discover/alexaDiscoveryDiscoverDirective.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alexa.Discovery/Discover/alexaDiscoveryDiscoverDirectivePayload.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alexa.Discovery/Discover.Response/alexaDiscoveryDiscoverResponseEvent.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alexa.Discovery/Discover.Response/alexaDiscoveryDiscoverResponseEventPayload.proto
)

set(NOTIFICATIONS_PROTO_FILES
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Notifications/SetIndicator/notificationsSetIndicatorDirective.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Notifications/SetIndicator/notificationsSetIndicatorDirectivePayload.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Notifications/ClearIndicator/notificationsClearIndicatorDirective.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Notifications/ClearIndicator/notificationsClearIndicatorDirectivePayload.proto
)

set(ALERTS_PROTO_FILES
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alerts/SetAlert/alertsSetAlertDirective.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alerts/SetAlert/alertsSetAlertDirectivePayload.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alerts/DeleteAlert/alertsDeleteAlertDirective.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alerts/DeleteAlert/alertsDeleteAlertDirectivePayload.proto
)

set(STATELISTENER_PROTO_FILES
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alexa.Gadget.StateListener/StateUpdate/alexaGadgetStateListenerStateUpdateDirective.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alexa.Gadget.StateListener/StateUpdate/alexaGadgetStateListenerStateUpdateDirectivePayload.proto
)

set(SPEECHDATA_PROTO_FILES
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alexa.Gadget.SpeechData/Speechmarks/alexaGadgetSpeechDataSpeechmarksDirective.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alexa.Gadget.SpeechData/Speechmarks/alexaGadgetSpeechDataSpeechmarksDirectivePayload.proto
)

set(MUSICDATA_PROTO_FILES
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alexa.Gadget.MusicData/Tempo/alexaGadgetMusicDataTempoDirective.proto
    ${ALEXA_EMBEDDED_PATH}/AlexaGadgetsProtobuf/Alexa.Gadget.MusicData/Tempo/alexaGadgetMusicDataTempoDirectivePayload.proto
)

set(CUSTOM_PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/custom_event.proto
)

foreach(GROUP BASE NOTIFICATIONS ALERTS STATELISTENER SPEECHDATA MUSICDATA CUSTOM)
    # Populate list of expected .c files output from protobuf generation
    foreach(file ${${GROUP}_PROTO_FILES})
        get_filename_component(FILE_DIR ${file} DIRECTORY)
        get_filename_component(FILE_BASE_NAME ${file} NAME_WE)
        list(APPEND PROTOC_INC_DIRS "-I.")
        list(APPEND PROTOC_INC_DIRS "-I${FILE_DIR}")
        list(APPEND ${GROUP}_PROTO_OUTPUTS "${PROTO_COMPILATION_DIR}/${FILE_BASE_NAME}.pb.c")
        list(APPEND ${GROUP}_PROTO_DEPENDENCIES "${file}")
    endforeach()

    # Each .pb.c file to be compiled by protoc gets its own CMake custom command,
    # with dependencies to all other .proto files in its group and the base files.
    foreach(file ${${GROUP}_PROTO_FILES})
        get_filename_component(FILE_DIR ${file} DIRECTORY)
        get_filename_component(FILE_NAME ${file} NAME)
        get_filename_component(FILE_BASE_NAME ${file} NAME_WE)

        set(PROTOC_ARGS "-I."
                        "${PROTOC_INC_DIRS}"
                        "--nanopb_out=${PROTO_COMPILATION_DIR}"
                        "${FILE_NAME}"
        )

        add_custom_command(
          OUTPUT "${PROTO_COMPILATION_DIR}/${FILE_BASE_NAME}.pb.c"
          COMMAND ${PROTOC_BIN} ${PROTOC_ARGS}
          WORKING_DIRECTORY ${FILE_DIR}
          MAIN_DEPENDENCY ${file}
          DEPENDS ${${GROUP}_PROTO_DEPENDENCIES} ${BASE_PROTO_DEPENDENCIES}
        )
    endforeach()
endforeach()

zephyr_library_named(alexa_gadgets_lib)

zephyr_include_directories(${PROTO_COMPILATION_DIR})
zephyr_include_directories(${NANOPB_PATH})

zephyr_library_sources_ifdef(CONFIG_BT_ALEXA_GADGETS
    ${CMAKE_CURRENT_SOURCE_DIR}/gadgets.c
)

zephyr_library_sources_ifdef(CONFIG_BT_ALEXA_GADGETS_PROFILE
    ${CMAKE_CURRENT_SOURCE_DIR}/gadgets_profile.c
    ${NANOPB_PATH}/pb_common.c
    ${NANOPB_PATH}/pb_encode.c
    ${NANOPB_PATH}/pb_decode.c
    ${BASE_PROTO_OUTPUTS}
)

zephyr_library_sources_ifdef(CONFIG_BT_ALEXA_GADGETS_CAPABILITY_NOTIFICATIONS
    ${NOTIFICATIONS_PROTO_OUTPUTS}
)

zephyr_library_sources_ifdef(CONFIG_BT_ALEXA_GADGETS_CAPABILITY_ALERTS
    ${ALERTS_PROTO_OUTPUTS}
)

zephyr_library_sources_ifdef(CONFIG_BT_ALEXA_GADGETS_CAPABILITY_STATELISTENER
    ${STATELISTENER_PROTO_OUTPUTS}
)

zephyr_library_sources_ifdef(CONFIG_BT_ALEXA_GADGETS_CAPABILITY_SPEECHDATA
    ${SPEECHDATA_PROTO_OUTPUTS}
)

zephyr_library_sources_ifdef(CONFIG_BT_ALEXA_GADGETS_CAPABILITY_MUSICDATA
    ${MUSICDATA_PROTO_OUTPUTS}
)

zephyr_library_sources_ifdef(CONFIG_BT_ALEXA_GADGETS_CAPABILITY_CUSTOM
    ${CUSTOM_PROTO_OUTPUTS}
)

#
# Give the main application access to the generated proto headers
#
# NOTE: This library corresponds to APP_LINK_WITH_ALEXA_GADGETS_PROTO in Kconfig
#       which is default set to "true".
#
zephyr_interface_library_named(alexa_gadgets_proto)
target_link_libraries(alexa_gadgets_proto INTERFACE alexa_gadgets_lib)
