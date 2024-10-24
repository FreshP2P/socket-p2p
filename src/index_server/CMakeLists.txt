project(socket-p2p-index)

add_executable(${PROJECT_NAME} index_server.c)
target_link_libraries(${PROJECT_NAME} shared)