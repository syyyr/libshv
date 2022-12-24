add_executable(minimalshvbroker
	src/main.cpp
	src/application.cpp
	)

target_link_libraries(minimalshvbroker libshvbroker)

install(TARGETS minimalshvbroker)
