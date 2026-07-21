add_executable(client
  main.cpp
  mainwindow.cpp
  mainwindow.h
  file_tools.cpp
  # file_tools.h
  console.cpp
  # console.h
)

target_link_libraries(client PRIVATE
  protocol
  Qt::Core
  Qt::Gui
  Qt::Network
  Qt::Widgets
  Qt::WebEngineWidgets
)
