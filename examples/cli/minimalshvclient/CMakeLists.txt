add_executable(minimalshvclient
	src/main.cpp
	src/application.cpp
	)

target_link_libraries(minimalshvclient libshviotqt)

install(TARGETS minimalshvclient)
