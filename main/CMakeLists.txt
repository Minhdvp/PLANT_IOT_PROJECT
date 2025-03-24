idf_component_register(SRCS "main.c" "src/mesh_handler.c" "src/wifi_handler.c" "src/nvs_handle.c" "src/http_server.c" "src/read_sensor.c" "src/AHT20.c" "src/soil_moisture.c" "src/connect_FB.c"
                    INCLUDE_DIRS "." "./inc" 
                    
                    EMBED_FILES "webserver/wificonfig.html" "webserver/firebase_cert.pem" )