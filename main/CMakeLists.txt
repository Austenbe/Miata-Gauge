idf_component_register(SRCS "main.c" "IO_EXPANDER/PCA9554A.c"
                    INCLUDE_DIRS "." "IO_EXPANDER"
                    REQUIRES esp_driver_i2c)

target_compile_definitions(${COMPONENT_LIB} PUBLIC)