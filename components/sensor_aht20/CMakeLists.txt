set(srcs "")
if(CONFIG_NODE_AHT20)
    list(APPEND srcs "sensor_aht20.c")
endif()

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "include"
                    REQUIRES esp_common
                    PRIV_REQUIRES node_core i2c_bus aht20)
