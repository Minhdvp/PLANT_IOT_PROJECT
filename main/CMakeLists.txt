idf_component_register(SRCS "main.c" "src/mesh_handler.c" "src/wifi_handler.c" "src/nvs_handle.c" "src/http_server.c"
                    INCLUDE_DIRS "." "./inc"
                    EMBED_FILES "webserver/wificonfig.html" )