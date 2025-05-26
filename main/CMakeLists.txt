idf_component_register(SRCS "main.c" "src/wifi_handler.c" "src/nvs_handle.c" "src/http_server.c" "src/read_sensor.c" "src/AHT20.c" "src/soil_moisture.c" "src/connect_FB.c" "wifiMesh/mesh_handler.c" "wifiMesh/mesh_send.c" "src/water_sensor.c" "src/fonts.c" "src/ssd1306.c" "src/oled_display.c" "src/mosfet_control.c"
                    INCLUDE_DIRS "." "./inc" "" "./wifiMesh/inc" 
                    
                    EMBED_FILES "webserver/wificonfig.html" "webserver/firebase_cert.pem"
                     )