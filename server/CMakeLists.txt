add_executable(server
  main.cpp
  compilerserver.cpp
  compilerserver.h
)

target_link_libraries(server PRIVATE
  protocol
  Qt::Core
  Qt::Network
  Qt::Sql
)
