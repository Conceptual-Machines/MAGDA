# MCP (Model Context Protocol) Domain
# This includes gRPC protocols, MCP server implementation, and agent management

# Generate protobuf and gRPC code
set(PROTO_FILES
    proto/mcp_service.proto
    proto/magica_daw.proto
)

# Create output directory for generated files
set(GENERATED_PROTOBUF_PATH ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_PROTOBUF_PATH})

# Generate protobuf files
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    
    set(PROTO_GENERATED_FILES
        ${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.cc
        ${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.h
        ${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.cc
        ${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.h
    )
    
    add_custom_command(
        OUTPUT ${PROTO_GENERATED_FILES}
        COMMAND protobuf::protoc
        ARGS --grpc_out=${GENERATED_PROTOBUF_PATH}
             --cpp_out=${GENERATED_PROTOBUF_PATH}
             --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
             -I${CMAKE_CURRENT_SOURCE_DIR}/proto
             ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
        COMMENT "Generating protobuf and gRPC files for ${PROTO_NAME}"
    )
    
    list(APPEND ALL_GENERATED_FILES ${PROTO_GENERATED_FILES})
endforeach()

# MCP server sources
set(MCP_SERVER_SOURCES
    server/grpc_mcp_server.cpp
    ${ALL_GENERATED_FILES}
)

set(MCP_SERVER_HEADERS
    server/grpc_mcp_server.hpp
    server/mcp_server_interface.hpp
)

# Create MCP (Model Context Protocol) library
add_library(magica_mcp STATIC ${MCP_SERVER_SOURCES})

# Link required libraries
target_link_libraries(magica_mcp
    PUBLIC
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    magica_daw
    PRIVATE
    Threads::Threads
)

# Include directories
target_include_directories(magica_mcp
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${GENERATED_PROTOBUF_PATH}
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Make sure protobuf files are generated before building
add_dependencies(magica_mcp ${ALL_GENERATED_FILES})

# Install headers
install(FILES ${MCP_SERVER_HEADERS} DESTINATION include/magica/mcp)

# Note: Agent executables in mcp/agents/ are meant to be run as separate processes
# They connect to the MCP server via gRPC and are not part of the main build 