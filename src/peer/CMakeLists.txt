project(socket-p2p-peer)

add_executable(${PROJECT_NAME} peer.c)
target_link_libraries(${PROJECT_NAME} shared)