#  Find `Editor` library.
#  Once done this will define:
#  UnigineExt::Editor - library target.
#  Editor_FOUND - library is found.
#  Editor_LIBRARY - library path.

if (TARGET Unigine::Editor)
	set(Editor_FOUND TRUE)
	return()
endif()

set(name "EditorCore")
if (UNIGINE_DOUBLE)
	string(APPEND name "_double")
endif ()
string(APPEND name "_x64")

find_library(Editor_LIBRARY_RELEASE
	NAMES ${name}
	PATHS ${UNIGINE_BIN_DIR} ${UNIGINE_LIB_DIR}
	NO_DEFAULT_PATH
	)

find_library(Editor_LIBRARY_DEBUG
	NAMES ${name}d
	PATHS ${UNIGINE_BIN_DIR} ${UNIGINE_LIB_DIR}
	NO_DEFAULT_PATH
	)

include(SelectLibraryConfigurations)
select_library_configurations(Editor)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Editor
	FOUND_VAR Editor_FOUND
	REQUIRED_VARS
		Editor_LIBRARY
		UNIGINE_INCLUDE_DIR
	)

if(Editor_FOUND)
	add_library(Unigine::Editor UNKNOWN IMPORTED)

	set_target_properties(Unigine::Editor
		PROPERTIES
		INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${UNIGINE_INCLUDE_DIR}
		INTERFACE_INCLUDE_DIRECTORIES ${UNIGINE_INCLUDE_DIR}
		)
	set_property(TARGET Unigine::Editor
		PROPERTY
		INTERFACE_COMPILE_DEFINITIONS $<$<BOOL:${UNIGINE_DOUBLE}>:UNIGINE_DOUBLE>
		)

	set_target_properties(Unigine::Editor
		PROPERTIES
		IMPORTED_LOCATION ${Editor_LIBRARY_RELEASE}
		IMPORTED_LOCATION_DEBUG ${Editor_LIBRARY_DEBUG}
		IMPORTED_LOCATION_RELEASE ${Editor_LIBRARY_RELEASE}
		)

endif()
