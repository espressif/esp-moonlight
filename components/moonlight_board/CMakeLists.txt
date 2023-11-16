if(CONFIG_ESP32_MOONLIGHT_BOARD)
set(srcs
    "src/board_esp32_moonlight.c"
)
elseif(CONFIG_ESP32_S3_MOONLIGHT_BOARD)
set(srcs
    "src/board_esp32s3_moonlight.c"
)
endif()

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "include"
                       REQUIRES nvs_flash
                       )

