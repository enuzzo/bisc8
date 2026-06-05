idf_component_register(
    SRCS
    "app_main.cpp"
    "app_config.cpp"
    "audio_service.cpp"
    "battery.cpp"
    "board_support.cpp"
    "button_controller.cpp"
    "captive_dns_service.cpp"
    "connectivity_service.cpp"
    "debug_serial.cpp"
    "display_service.cpp"
    "fortune_service.cpp"
    "generated/logo_assets.cpp"
    "generated/sound_assets.cpp"
    "localization.cpp"
    "email_service.cpp"
    "voice_oracle_service.cpp"
    "web_portal.cpp"
    INCLUDE_DIRS 
    "./"
    PRIV_REQUIRES
    esp_adc
    esp_driver_gpio
    esp_event
    esp_http_client
    esp_http_server
    esp_netif
    esp_timer
    esp_wifi
    json
    lwip
    mbedtls
    espressif__esp_io_expander
    espressif__esp_io_expander_tca9554
    REQUIRES
    esp_driver_usb_serial_jtag
    my_button
    nvs_flash
    port_bsp)

# --- Generated build version header (r<NNNN>-<YYYYMMDD>-<HHMM>[+]) ---
set(BISC8_BUILD_INFO_DIR "${CMAKE_CURRENT_BINARY_DIR}/build_info")
set(BISC8_BUILD_INFO_HEADER "${BISC8_BUILD_INFO_DIR}/build_info.h")
file(MAKE_DIRECTORY "${BISC8_BUILD_INFO_DIR}")

# Generate once at configure time so the header exists for the first compile,
# then regenerate on every build so the timestamp tracks the actual build.
execute_process(COMMAND "${CMAKE_COMMAND}"
    -DOUTPUT_FILE=${BISC8_BUILD_INFO_HEADER}
    -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/build_info.cmake)
add_custom_target(bisc8_build_info
    COMMAND "${CMAKE_COMMAND}"
        -DOUTPUT_FILE=${BISC8_BUILD_INFO_HEADER}
        -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/build_info.cmake
    BYPRODUCTS "${BISC8_BUILD_INFO_HEADER}"
    COMMENT "Generating Bisc8 build version"
    VERBATIM)
add_dependencies(${COMPONENT_LIB} bisc8_build_info)
target_include_directories(${COMPONENT_LIB} PRIVATE "${BISC8_BUILD_INFO_DIR}")
