project(socket-p2p-shared)

add_library(${PROJECT_NAME}
  config/constants.h
  pdu/pdu.h
  hash_table/hashtable.h
  hash_table/hashtable.c
  linked_list/contentlist.h
  linked_list/contentlist.c
)
add_library(shared ALIAS ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)

target_include_directories(${PROJECT_NAME}
  PUBLIC ${PROJECT_SOURCE_DIR}
)