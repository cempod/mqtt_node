idf_component_register(SRCS "main.c"
                            "aht20_sensor.c"
                            "net/wifi.c"
                            "net/mqtt.c"
                            "net/ha_discovery.c"
                            "system/led_indicator.c"
                            "system/device_id.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif esp_wifi esp_driver_gpio esp_driver_ledc i2c_bus
                    INCLUDE_DIRS "."
                                "net"
                                "system"
                    EMBED_TXTFILES "net/client.crt" "net/client.key" "net/ca.crt")