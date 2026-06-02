idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver External_communication inside_communication Data_declaration sensor esp_timer)
