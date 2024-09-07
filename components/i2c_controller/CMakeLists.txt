idf_component_register(SRCS "sub.c" "dom.c" "i2c_controller.c"
                    PRIV_REQUIRES esp_driver_i2c
                    INCLUDE_DIRS "include")
