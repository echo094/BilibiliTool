
init_target("ToolConfig")

file (GLOB HEADER_FILES *.h)
file (GLOB SOURCE_FILES *.cpp)
file (GLOB RESOURCE_FILES *.rc)
file (GLOB RES_FOLDER "res/*.rc2" "res/*.ico")
set (RESOURCE_FILES ${RESOURCE_FILES} ${RES_FOLDER})
source_group("Resource Files" FILES ${RESOURCE_FILES})

find_package(MFC)
add_definitions(-D_AFXDLL)
set(CMAKE_MFC_FLAG 2)
build_mfc (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES} ${APP_SOURCE_FILES} ${APP_HEADER_FILES})

final_target ()

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "app")
