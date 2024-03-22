#
add_definitions(-UNDEBUG)

add_executable(test-controller test_controller.cpp)
target_link_libraries(test-controller yaml-cpp nacs-utils nidaq)
set_source_files_properties(test_controller.cpp
  PROPERTIES COMPILE_FLAGS "-ffp-contract=fast")

add_executable(test-server test_server.cpp)
target_link_libraries(test-server yaml-cpp nacs-utils nidaq PkgConfig::ZMQ)
set_source_files_properties(test_server.cpp
  PROPERTIES COMPILE_FLAGS "-ffp-contract=fast")

add_executable(test-client test_client.cpp)
target_link_libraries(test-client nacs-utils PkgConfig::ZMQ)
set_source_files_properties(test_server.cpp
  PROPERTIES COMPILE_FLAGS "-ffp-contract=fast")
