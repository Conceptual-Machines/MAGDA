# Generate protobuf and gRPC code
set(PROTO_FILES
    ${CMAKE_SOURCE_DIR}/proto/mcp_service.proto
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
             -I${CMAKE_SOURCE_DIR}/proto
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating protobuf and gRPC files for ${PROTO_NAME}"
    )
    
    list(APPEND ALL_GENERATED_FILES ${PROTO_GENERATED_FILES})
endforeach()

# Core library sources
set(MAGDA_CORE_SOURCES
    core/command.cpp
    core/grpc_mcp_server.cpp
    magda.cpp
    ${ALL_GENERATED_FILES}
)

# Create the core library
add_library(magda_core STATIC ${MAGDA_CORE_SOURCES})

# Link libraries
target_link_libraries(magda_core 
    PRIVATE 
    nlohmann_json::nlohmann_json
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    Threads::Threads
)

# Include directories
target_include_directories(magda_core
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${GENERATED_PROTOBUF_PATH}
)

# Make sure protobuf files are generated before building
add_dependencies(magda_core ${ALL_GENERATED_FILES}) 