set(_NAME splint-maker)
set(PLUGIN_NAME ${_NAME} PARENT_SCOPE)
message("Configuring ${_NAME}")

file(GLOB ${_NAME}_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)
file(GLOB ${_NAME}_HDR CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_library (${_NAME} SHARED
    ${${_NAME}_SRC}
    ${${_NAME}_HDR}
)

target_compile_definitions(${_NAME} PRIVATE
	DPVISION_PLUGIN_DLL
    DPVISION_LEGACY_API_STATIC
    NOMINMAX
)

target_link_libraries (${_NAME} 
	dpVision::Core
    dpVision::Gui
    dpVision::LegacyApi
    dpVision::PluginManager
)

target_include_directories (${_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
	${dpVision_INCLUDE_DIR}
)

if (MSVC)
	set_target_properties( ${_NAME} PROPERTIES
		FOLDER "plugins"
		LINKER_LANGUAGE CXX
	)
	add_custom_command(TARGET ${_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:dpVision>/plugins
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${_NAME}> $<TARGET_FILE_DIR:dpVision>/plugins/
	)
    target_compile_options(${_NAME} PRIVATE /bigobj)
else (MSVC)
	set_target_properties(${_NAME} PROPERTIES
		LINKER_LANGUAGE CXX
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
	)
endif (MSVC)
