set(PARSE_HEADERS "")
set(PARSE_SOURCES parse.cpp)

if (WIN32)
    add_executable(parse WIN32 ${PARSE_HEADERS} ${PARSE_SOURCES})
else()
    add_executable(parse ${PARSE_SOURCES})
endif()
