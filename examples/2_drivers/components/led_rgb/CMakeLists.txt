set(component_srcs "led_rgb.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES "")

