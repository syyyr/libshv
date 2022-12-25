add_executable(samplegraph
	main.cpp
	mainwindow.cpp
	)

target_link_libraries(samplegraph PRIVATE libshvvisu)

target_include_directories(samplegraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS samplegraph)
