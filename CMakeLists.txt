cmake_minimum_required(VERSION 3.21)

project(veyon-internet-access-control
	VERSION 1.0.0
	DESCRIPTION "Veyon plugin for blocking internet access on managed computers"
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(IAC_BUILD_TESTS "Build unit tests" OFF)

set(VEYON_VERSION "4.11.4" CACHE STRING
	"Veyon release whose headers are downloaded if VEYON_SOURCE_DIR is not set")
set(VEYON_SOURCE_DIR "" CACHE PATH
	"Veyon source tree providing the headers (core/src); must match the installed Veyon minor version")
set(VEYON_LIBRARY_DIR "" CACHE PATH
	"Directory containing libveyon-core.so / veyon-core.dll (searched in default locations if empty)")
set(VEYON_PLUGIN_DIR "" CACHE PATH
	"Plugin install directory (defaults to the directory containing the Veyon core library)")

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Network LinguistTools)

# Veyon headers

if(NOT VEYON_SOURCE_DIR)
	set(VEYON_SOURCE_DIR "${CMAKE_BINARY_DIR}/veyon-${VEYON_VERSION}")
	if(NOT EXISTS "${VEYON_SOURCE_DIR}/core/src/VeyonCore.h")
		set(archive "${CMAKE_BINARY_DIR}/veyon-${VEYON_VERSION}.tar.gz")
		message(STATUS "Downloading Veyon ${VEYON_VERSION} sources for headers")
		file(DOWNLOAD "https://github.com/veyon/veyon/archive/refs/tags/v${VEYON_VERSION}.tar.gz" "${archive}"
			STATUS download_status TLS_VERIFY ON)
		list(GET download_status 0 download_result)
		if(NOT download_result EQUAL 0)
			message(FATAL_ERROR "Failed to download Veyon sources: ${download_status}. Set VEYON_SOURCE_DIR instead.")
		endif()
		file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${CMAKE_BINARY_DIR}")
	endif()
endif()

set(VEYON_INCLUDE_DIR "${VEYON_SOURCE_DIR}/core/src")
if(NOT EXISTS "${VEYON_INCLUDE_DIR}/VeyonCore.h")
	message(FATAL_ERROR "Veyon headers not found in ${VEYON_INCLUDE_DIR}")
endif()

# Veyon core library

if(WIN32)
	# MinGW can link directly against the DLL shipped with the Veyon installer
	list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".dll")
endif()

if(VEYON_LIBRARY_DIR)
	# use the given directory as is, also when cross compiling with a sysroot
	find_library(VEYON_CORE_LIBRARY
		NAMES veyon-core
		PATHS "${VEYON_LIBRARY_DIR}"
		NO_DEFAULT_PATH
		NO_CMAKE_FIND_ROOT_PATH)
else()
	find_library(VEYON_CORE_LIBRARY
		NAMES veyon-core
		PATHS
			/usr/lib64/veyon
			/usr/lib/veyon
			/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/veyon
			"C:/Program Files/Veyon"
		NO_CMAKE_SYSTEM_PATH)
endif()

if(NOT VEYON_CORE_LIBRARY)
	message(FATAL_ERROR "Veyon core library not found - install Veyon or set VEYON_LIBRARY_DIR")
endif()
message(STATUS "Using Veyon core library: ${VEYON_CORE_LIBRARY}")

if(NOT VEYON_PLUGIN_DIR)
	if(WIN32)
		set(VEYON_PLUGIN_DIR "plugins")
	else()
		# Veyon loads its plugins from the directory containing libveyon-core.so on all distributions
		get_filename_component(VEYON_PLUGIN_DIR "${VEYON_CORE_LIBRARY}" DIRECTORY)
	endif()
endif()

# Plugin

add_library(internetaccesscontrol MODULE
	src/InternetAccessControlConfiguration.h
	src/InternetAccessControlConfigurationPage.cpp
	src/InternetAccessControlConfigurationPage.h
	src/InternetAccessControlConfigurationPage.ui
	src/InternetAccessControlFeaturePlugin.cpp
	src/InternetAccessControlFeaturePlugin.h
	src/FirewallBackend.h
	src/FirewallController.cpp
	src/FirewallController.h
	src/NetworkRanges.cpp
	src/NetworkRanges.h
	src/backends/NoopFirewallBackend.h
	resources/internetaccesscontrol.qrc)

# Firewall backends - see CONTRIBUTING.md for adding one for another platform
if(WIN32)
	target_sources(internetaccesscontrol PRIVATE
		src/backends/WindowsFirewallBackend.cpp
		src/backends/WindowsFirewallBackend.h)
	target_link_libraries(internetaccesscontrol PRIVATE ole32 oleaut32 uuid)
	if(MINGW)
		# the Windows Firewall GUIDs are defined via <initguid.h> and also provided by libuuid
		target_link_options(internetaccesscontrol PRIVATE "-Wl,--allow-multiple-definition")
	endif()
endif()

set_target_properties(internetaccesscontrol PROPERTIES PREFIX "")

target_compile_definitions(internetaccesscontrol PRIVATE
	PLUGIN_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
	PLUGIN_VERSION_MINOR=${PROJECT_VERSION_MINOR}
	PLUGIN_VERSION_PATCH=${PROJECT_VERSION_PATCH})

target_include_directories(internetaccesscontrol PRIVATE
	src
	src/compat
	${VEYON_INCLUDE_DIR})

target_link_libraries(internetaccesscontrol PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	Qt6::Network
	"${VEYON_CORE_LIBRARY}")

qt_add_translations(internetaccesscontrol
	TS_FILES translations/internetaccesscontrol_uk.ts
	RESOURCE_PREFIX /internetaccesscontrol/translations
	LUPDATE_OPTIONS -no-obsolete -locations none)

install(TARGETS internetaccesscontrol
	LIBRARY DESTINATION "${VEYON_PLUGIN_DIR}"
	RUNTIME DESTINATION "${VEYON_PLUGIN_DIR}")

# Tests

if(IAC_BUILD_TESTS)
	enable_testing()
	find_package(Qt6 REQUIRED COMPONENTS Test)

	add_executable(networkrangestest tests/NetworkRangesTest.cpp src/NetworkRanges.cpp)
	target_include_directories(networkrangestest PRIVATE src)
	target_link_libraries(networkrangestest PRIVATE Qt6::Network Qt6::Test)
	add_test(NAME networkrangestest COMMAND networkrangestest)
endif()
