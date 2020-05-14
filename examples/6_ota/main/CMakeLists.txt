
# Embed the server root certificate into the final binary
idf_build_get_property(project_dir PROJECT_DIR)
idf_component_register(SRCS "app_main.c"
                            "ota.c"
                    EMBED_TXTFILES ${project_dir}/https_server/ca_cert.pem)