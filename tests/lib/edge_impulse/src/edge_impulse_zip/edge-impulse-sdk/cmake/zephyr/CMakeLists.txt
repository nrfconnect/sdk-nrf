set(EI_SDK_FOLDER ../../)

target_include_directories(app PRIVATE
    ${EI_SDK_FOLDER}
    ${EI_SDK_FOLDER}/..
)

target_sources(app PRIVATE ${EI_SDK_FOLDER}/ei_run_classifier_mock.cpp)
