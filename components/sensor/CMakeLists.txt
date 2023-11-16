set(component_srcs "sensor.c")

if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "5.0")
    list(APPEND REQ esp_adc)
else()
    list(APPEND REQ esp_adc_cal)
endif()

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES driver esp_timer ${REQ}
                       )

