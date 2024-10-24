project(socket-p2p-shared)

add_library(${PROJECT_NAME} pdu/pdu.h)
add_library(shared ALIAS ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)

target_include_directories(${PROJECT_NAME}
  PUBLIC ${PROJECT_SOURCE_DIR}
)