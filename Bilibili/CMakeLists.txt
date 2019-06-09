
init_target("Bilibili")

file (GLOB HEADER_FILES *.h)
file (GLOB SOURCE_FILES *.cpp)
file (GLOB RESOURCE_FILES *.rc)
source_group("Resource Files" FILES ${RESOURCE_FILES})
add_source_folder("event")
add_source_folder("source")
add_source_folder("tcpio")
add_source_folder("utility")

build_executable (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES} ${APP_SOURCE_FILES} ${APP_HEADER_FILES})

link_boost ()
link_curl ()
link_openssl ()
link_rapidjson ()
link_websocketpp ()
link_zlib ()

final_target ()

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "app")

if (MSVC)
  list (APPEND dllfiles
  ${CURL_DLL}
  "${OPENSSL_BIN_DIR}/libcrypto-1_1.dll"
  "${OPENSSL_BIN_DIR}/libssl-1_1.dll"
  "${ZLIB_BIN_DIR}/zlib.dll" )
  file(COPY ${dllfiles}
  DESTINATION  "${CMAKE_CURRENT_BINARY_DIR}/")
  install(FILES ${dllfiles}
  CONFIGURATIONS Debug
  DESTINATION  "Debug/bin")
  install(FILES ${dllfiles}
  CONFIGURATIONS Release
  DESTINATION  "Release/bin")
endif ()
