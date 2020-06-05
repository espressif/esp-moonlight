set(component_srcs "blufi.c"
                   "blufi_security.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES nvs_flash bt
                       )