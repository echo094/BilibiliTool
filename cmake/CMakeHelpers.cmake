
# Adds the given folder_name into the source files of the current project.
# Use this macro when your module contains .cpp and .h files in several subdirectories.
# Your sources variable needs to be APP_SOURCE_FILES and headers variable APP_HEADER_FILES.
macro(add_source_folder folder_name)
    file(GLOB H_FILES_IN_FOLDER_${folder_name} ${folder_name}/*.hpp ${folder_name}/*.h)
    file(GLOB CPP_FILES_IN_FOLDER_${folder_name} ${folder_name}/*.cpp ${folder_name}/*.c)
    source_group("${folder_name}" FILES ${H_FILES_IN_FOLDER_${folder_name}})
    source_group("${folder_name}" FILES ${CPP_FILES_IN_FOLDER_${folder_name}})
    set(APP_HEADER_FILES ${APP_HEADER_FILES} ${H_FILES_IN_FOLDER_${folder_name}})
    set(APP_SOURCE_FILES ${APP_SOURCE_FILES} ${CPP_FILES_IN_FOLDER_${folder_name}})
endmacro()

# Initialize target.
macro (init_target NAME)
    set (TARGET_NAME ${NAME})
    message ("** " ${TARGET_NAME})

    # Include our own module path. This makes #include "x.h"
    # work in project subfolders to include the main directory headers.
    include_directories (${CMAKE_CURRENT_SOURCE_DIR})
endmacro ()

# Build executable for executables
macro (build_executable TARGET_NAME)
    set (TARGET_LIB_TYPE "EXECUTABLE")
    message (STATUS "-- Build Type:" ${TARGET_LIB_TYPE})

    add_executable (${TARGET_NAME} ${ARGN})

    set_target_properties (${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${APP_BIN})
    set_target_properties (${TARGET_NAME} PROPERTIES DEBUG_POSTFIX d)
endmacro ()

# Build mfc for executables
macro (build_mfc TARGET_NAME)
    set (TARGET_LIB_TYPE "EXECUTABLE")
    message (STATUS "-- Build Type:" ${TARGET_LIB_TYPE})

    add_executable (${TARGET_NAME} WIN32 ${ARGN})

    set_target_properties (${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${APP_BIN})
    set_target_properties (${TARGET_NAME} PROPERTIES DEBUG_POSTFIX d)
endmacro ()

# Build library
macro (build_library TARGET_NAME)
    set (TARGET_LIB_TYPE "LIBRARY")
    message (STATUS "-- Build Type:" ${TARGET_LIB_TYPE})

    add_library (${TARGET_NAME} SHARED ${ARGN})

    set_target_properties (${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${APP_BIN})
    set_target_properties (${TARGET_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${APP_LIB})
    set_target_properties (${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${APP_LIB})
    set_target_properties (${TARGET_NAME} PROPERTIES DEBUG_POSTFIX d)
endmacro ()

# Build executable and register as test
macro (build_test TARGET_NAME)
    build_executable (${TARGET_NAME} ${ARGN})

    if (${CMAKE_VERSION} VERSION_LESS 3)
        message(WARNING "CMake too old to register ${TARGET_NAME} as a test")
    else ()
        add_test(NAME ${TARGET_NAME} COMMAND $<TARGET_FILE:${TARGET_NAME}>)
    endif ()
endmacro ()

# Finalize target for all types
macro (final_target)
    if ("${TARGET_LIB_TYPE}" STREQUAL "EXECUTABLE")
        install (TARGETS ${TARGET_NAME}
                 RUNTIME DESTINATION "bin"
                 CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES})
    endif ()

    if ("${TARGET_LIB_TYPE}" STREQUAL "LIBRARY")
        install (TARGETS ${TARGET_NAME}
                 RUNTIME DESTINATION "bin"
                 ARCHIVE DESTINATION "lib"
                 LIBRARY DESTINATION "lib"
                 CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES})
    endif ()
endmacro ()

macro (link_boost)
    target_link_libraries (${TARGET_NAME} ${Boost_LIBRARIES})
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})
endmacro ()

macro (link_curl)
    target_link_libraries (${TARGET_NAME} ${CURL_LIBRARIES})
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES ${CURL_INCLUDE_DIRS})
endmacro ()

macro (link_openssl)
    target_link_libraries (${TARGET_NAME} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR})
endmacro ()

macro (link_rapidjson)
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES ${RAPIDJSON_INCLUDE_DIR})
endmacro ()

macro (link_sqlite3)
    target_link_libraries (${TARGET_NAME} ${SQLite3_LIBRARIES})
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES ${SQLite3_INCLUDE_DIRS})
endmacro ()

macro (link_websocketpp)
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES ${WEPP_INCLUDE_DIR})
endmacro ()

macro (link_zlib)
	target_link_libraries (${TARGET_NAME} ${ZLIB_LIBRARIES})
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES ${ZLIB_INCLUDE_DIRS})
endmacro ()

macro (include_subdirs PARENT)
    file (GLOB SDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${PARENT}/*")
    foreach (SUBDIR ${SDIRS})
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/CMakeLists.txt")
            add_subdirectory ("${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}")
        endif ()
    endforeach ()
endmacro()
