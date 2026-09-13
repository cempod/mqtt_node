idf_component_register(SRCS "main.c"
                            "net/wifi.c"
                            "net/mqtt.c"
                            "system/led_indicator.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif esp_wifi
                    INCLUDE_DIRS "."
                                "net"
                                "system"
                    EMBED_TXTFILES "net/client.crt" "net/client.key" "net/ca.crt")