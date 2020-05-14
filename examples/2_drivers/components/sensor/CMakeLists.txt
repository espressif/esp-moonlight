set(component_srcs "sensor.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES driver esp_adc_cal
                       )

