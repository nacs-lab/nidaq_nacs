#

set(nidaq_HDRS
  Config.h
  Controller.h
  SeqCache.h
  Server.h
  )
set(nidaq_SRCS
  Config.cpp
  Controller.cpp
  SeqCache.cpp
  Server.cpp
  )

set(nidaq_LINKS ${SLEEF_LIBRARIES} ${NIDAQ} ${DEPS_LIBRARIES})
set_source_files_properties(Controller.cpp
  PROPERTIES COMPILE_FLAGS "-ffp-contract=fast")
add_definitions("-\"DNACS_EXPORT_LIB_nidaq()=\"")
set_property(SOURCE ${nidaq_SRCS} APPEND_STRING
  PROPERTY COMPILE_FLAGS " -Wno-conversion")

add_library(nidaq SHARED
  ${nidaq_SRCS})
target_link_libraries(nidaq PUBLIC ${nidaq_LINKS})
target_link_libraries(nidaq PRIVATE yaml-cpp PkgConfig::ZMQ)
set_target_properties(nidaq PROPERTIES
  VERSION "${MAJOR_VERSION}.${MINOR_VERSION}"
  SOVERSION "${MAJOR_VERSION}"
  COMPILE_FLAGS "-fvisibility=hidden"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS nidaq
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
