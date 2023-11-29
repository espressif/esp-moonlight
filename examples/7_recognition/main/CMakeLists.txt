
idf_component_register(SRCS "app_main.c" "app_speech_rec.c"
                       INCLUDE_DIRS "include")

if(CONFIG_ESP32_S3_MOONLIGHT_BOARD)
    spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
endif()