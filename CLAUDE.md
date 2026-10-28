# mqtt_node
Прошивка ESP32-C3: датчик AHT20 → MQTT (mTLS) → Home Assistant через MQTT Discovery.

## Стек
- C, ESP-IDF v6.0.2, target esp32c3
- Компоненты (idf_component.yml): espressif/mqtt, espressif/aht20 (+i2c_bus), espressif/cjson, espressif/led_strip

## Структура
- `main/main.c` — старт: LED, NVS, Wi-Fi, MQTT, задача датчика, цикл индикации
- `main/aht20_sensor.*` — I2C + AHT20, задача публикации
- `main/net/` — wifi, mqtt (LWT/online, discovery при подключении), ha_discovery (cJSON)
- `main/system/` — device_id (имя + MAC), led_indicator (RGB/PWM/GPIO по Kconfig)
- `main/Kconfig.projbuild` — имя узла, URI брокера, Wi-Fi, тип/пины LED

## Команды
- Окружение: `. ~/.espressif/tools/activate_idf_v6.0.2.sh`
- Сборка: `idf.py build`
- Прошивка: `idf.py -p /dev/ttyACM0 flash`
- Лог: `idf.py monitor` интерактивный — для захвата UART использовать pyserial-скрипт
- Тесты: нет

## Правила проекта
- `main/net/{ca.crt,client.crt,client.key}` встраиваются в прошивку (EMBED_TXTFILES); в git не попадают — никогда не коммитить.
- `sdkconfig` не в git (там пароль Wi-Fi).
- Ноутбук не достаёт до брокера (другая подсеть/VPN) — работу с HA проверяет пользователь с телефона.
- Код без комментариев.

Планы и статус — в `.notes/plan.md`.
