set(srcs 
  "app_main.c"
  "laso_util.c"
  "max7219.c"
  "breakbeam.c"
  "websocket.c"
  "connect.c"
)

idf_component_register(
  SRCS ${srcs}
  INCLUDE_DIRS "."
)
