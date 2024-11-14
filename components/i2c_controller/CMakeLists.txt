idf_component_register(SRCS "i2c_master.c" "devices.c" "utils.c" "i2c_slave.c" "i2c_messages.c" "sub.c" "dom.c" "i2c_controller.c"
                    PRIV_REQUIRES esp_driver_i2c
                    INCLUDE_DIRS "include")
