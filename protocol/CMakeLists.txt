add_library(protocol STATIC
  message.cpp
  message.h
)

target_include_directories(protocol PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(protocol PUBLIC
  Qt::Core
  Qt::Network
)
