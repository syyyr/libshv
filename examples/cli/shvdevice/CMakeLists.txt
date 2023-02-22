add_executable(shvdevice
	src/main.cpp
	src/appclioptions.cpp
	src/application.cpp
	)

target_link_libraries(shvdevice libshviotqt)

install(TARGETS shvdevice)
