set(cert_dir "${PROJECT_DIR}/certs")
set(cert_group_dir "${cert_dir}/${CONFIG_NODE_CERT_GROUP}")

idf_component_register(SRCS "node.c"
                            "wifi.c"
                            "mqtt.c"
                            "ha_discovery.c"
                            "device_id.c"
                            "led_indicator.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    PRIV_REQUIRES mqtt nvs_flash esp_netif esp_wifi esp_timer esp_app_format
                                  esp_driver_gpio esp_driver_ledc cjson
                    EMBED_TXTFILES "${cert_dir}/ca.crt"
                                   "${cert_group_dir}/client.crt"
                                   "${cert_group_dir}/client.key")
