cmake_minimum_required(VERSION 3.14)
set(proj_name GeometryNodesPlugin_x64)
if (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
	set(proj_name GeometryNodesPlugin_x64d)
endif()
project(${proj_name} CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTORCC TRUE)

##==============================================================================
## Environment.
##==============================================================================
set(UNIGINE_DOUBLE TRUE)
set(UNIGINE_SDK_PATH ${PROJECT_SOURCE_DIR}/../../)
set(UNIGINE_BIN_DIR ${UNIGINE_SDK_PATH}bin/)
set(UNIGINE_LIB_DIR ${UNIGINE_SDK_PATH}lib/)
set(UNIGINE_INCLUDE_DIR ${UNIGINE_SDK_PATH}include/)
set(UNIGINE_OUTPUT_EDITOR_RELEASE_DIR ${UNIGINE_BIN_DIR}editor/)
set(UNIGINE_OUTPUT_EDITOR_DEBUG_DIR ${UNIGINE_BIN_DIR}editor_debug/)

##==============================================================================
## Dependencies.
##==============================================================================
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules)
find_package(Engine REQUIRED MODULE QUIET)
find_package(Editor REQUIRED MODULE QUIET)

# UNIGINE_QTROOT is Unigine related global variable that points to the effective
# Qt's distributive.
set(QT5_ROOT "$ENV{UNIGINE_QTROOT}")
if (QT5_ROOT AND EXISTS "${QT5_ROOT}")
	list(APPEND CMAKE_PREFIX_PATH ${QT5_ROOT})
endif()

# The following construct is required as Editor's Debug build
# uses Release binaries of QT5 framework
set(temp_map_imported ${CMAKE_MAP_IMPORTED_CONFIG_DEBUG})
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG RELEASE)
# Find additional required packages
find_package(Qt5 5.12.3 CONFIG EXACT COMPONENTS Core Gui Widgets)
# Restore the actual value
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG ${temp_map_imported})
unset(temp_map_imported)

if (NOT Qt5_FOUND)
	message(FATAL_ERROR "The required Qt version is not found on your computer, "
	"please follow this link to see the instructions on resolving this issue. "
	"https://developer.unigine.com/en/docs/latest/editor2/extensions/environment")
endif()

##==============================================================================
## Target.
##==============================================================================

add_library(${proj_name} SHARED
	${CMAKE_CURRENT_SOURCE_DIR}/Gui/AbstractSpinBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/ColorWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/ImageWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/MainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/NodeWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/SpinBoxDouble.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/SpinBoxInt.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/TripleSpinBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui/bgn_resources.qrc
	${CMAKE_CURRENT_SOURCE_DIR}/BlendAsset.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/GeometryNodesPlugin.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/BlenderProcess.cpp
)

target_include_directories(${proj_name} SYSTEM
	PRIVATE
	${UNIGINE_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/Gui
	)

target_link_libraries(${proj_name}
	PRIVATE
	Unigine::Engine
	Unigine::Editor
	Qt5::Core
	Qt5::Gui
	Qt5::Widgets
	)

if (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    target_compile_definitions(${proj_name}
		PRIVATE
		DEBUG
		# QT_DEBUG
	)
else()
	target_compile_definitions(${proj_name}
		PRIVATE
		NDEBUG
		# QT_NO_DEBUG
	)
endif()


set_target_properties(${proj_name}
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${UNIGINE_OUTPUT_EDITOR_RELEASE_DIR}
	LIBRARY_OUTPUT_DIRECTORY_RELEASE ${UNIGINE_OUTPUT_EDITOR_RELEASE_DIR}
	# ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../../lib/editor/
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${UNIGINE_OUTPUT_EDITOR_DEBUG_DIR}
	LIBRARY_OUTPUT_DIRECTORY_DEBUG ${UNIGINE_OUTPUT_EDITOR_DEBUG_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${UNIGINE_OUTPUT_EDITOR_DEBUG_DIR}
	LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${UNIGINE_OUTPUT_EDITOR_DEBUG_DIR}
	# ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/../../lib/editor_debug/
	C_VISIBILITY_PRESET hidden
	CXX_VISIBILITY_PRESET hidden
)

if (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
	configure_file(${PROJECT_SOURCE_DIR}/geonodes.py ${UNIGINE_OUTPUT_EDITOR_DEBUG_DIR}/geonodes.py COPYONLY)
else()
	configure_file(${PROJECT_SOURCE_DIR}/geonodes.py ${UNIGINE_OUTPUT_EDITOR_RELEASE_DIR}/geonodes.py COPYONLY)
endif()
