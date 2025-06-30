#include "grpc_mcp_server.hpp"
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>
#include <random>
#include <sstream>
#include <iostream>
#include <ctime>

GrpcMCPServer::GrpcMCPServer(int port) : port_(port), running_(false) {}

GrpcMCPServer::~GrpcMCPServer() {
    stop();
}

bool GrpcMCPServer::start() {
    if (running_.load()) {
        return true;
    }
    
    try {
        grpc::ServerBuilder builder;
        
        // Listen on the given address without any authentication mechanism
        std::string server_address = "0.0.0.0:" + std::to_string(port_);
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        
        // Register this service
        builder.RegisterService(this);
        
        // Assemble the server
        server_ = builder.BuildAndStart();
        
        if (!server_) {
            std::cerr << "Failed to start gRPC server" << std::endl;
            return false;
        }
        
        running_.store(true);
        std::cout << "gRPC MCP Server started on " << server_address << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to start gRPC server: " << e.what() << std::endl;
        return false;
    }
}

void GrpcMCPServer::stop() {
    if (!running_.load()) {
        return;
    }
    
    try {
        if (server_) {
            server_->Shutdown();
            server_->Wait();  // Wait for all RPCs to finish
            server_.reset();
        }
        
        {
            std::lock_guard<std::mutex> lock(agents_mutex_);
            agents_.clear();
        }
        
        running_.store(false);
        std::cout << "gRPC MCP Server stopped" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error stopping gRPC server: " << e.what() << std::endl;
    }
}

void GrpcMCPServer::registerCommandHandler(const std::string& command_type, CommandHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    command_handlers_[command_type] = handler;
    std::cout << "Registered handler for command: " << command_type << std::endl;
}

void GrpcMCPServer::broadcastMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(agents_mutex_);
    
    magda::mcp::CommandResponse response;
    response.set_status(magda::mcp::CommandResponse::SUCCESS);
    response.set_data(message);
    
    for (const auto& [agent_id, agent] : agents_) {
        if (agent->stream) {
            try {
                agent->stream->Write(response);
            } catch (const std::exception& e) {
                std::cerr << "Failed to send message to agent " << agent_id << ": " << e.what() << std::endl;
            }
        }
    }
}

void GrpcMCPServer::sendToAgent(const std::string& agent_id, const std::string& message) {
    auto agent = getAgent(agent_id);
    if (agent && agent->stream) {
        magda::mcp::CommandResponse response;
        response.set_status(magda::mcp::CommandResponse::SUCCESS);
        response.set_data(message);
        
        try {
            agent->stream->Write(response);
        } catch (const std::exception& e) {
            std::cerr << "Failed to send message to agent " << agent_id << ": " << e.what() << std::endl;
        }
    }
}

std::vector<std::string> GrpcMCPServer::getConnectedAgents() const {
    std::lock_guard<std::mutex> lock(agents_mutex_);
    
    std::vector<std::string> agent_ids;
    for (const auto& [agent_id, agent] : agents_) {
        agent_ids.push_back(agent_id);
    }
    return agent_ids;
}

size_t GrpcMCPServer::getAgentCount() const {
    std::lock_guard<std::mutex> lock(agents_mutex_);
    return agents_.size();
}

grpc::Status GrpcMCPServer::ExecuteCommand(grpc::ServerContext* context,
                                          const magda::mcp::CommandRequest* request,
                                          magda::mcp::CommandResponse* response) {
    try {
        Command command = convertFromProto(*request);
        CommandResponse cmd_response = executeCommand(command);
        convertToProto(cmd_response, response);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_status(magda::mcp::CommandResponse::ERROR);
        response->set_message("Command execution failed: " + std::string(e.what()));
        return grpc::Status::OK;
    }
}

grpc::Status GrpcMCPServer::CommandStream(grpc::ServerContext* context,
                                         grpc::ServerReaderWriter<magda::mcp::CommandResponse,
                                                                 magda::mcp::CommandRequest>* stream) {
    magda::mcp::CommandRequest request;
    
    while (stream->Read(&request)) {
        try {
            Command command = convertFromProto(request);
            CommandResponse cmd_response = executeCommand(command);
            
            magda::mcp::CommandResponse response;
            convertToProto(cmd_response, &response);
            
            if (!stream->Write(response)) {
                break;  // Stream is closed
            }
            
        } catch (const std::exception& e) {
            magda::mcp::CommandResponse error_response;
            error_response.set_status(magda::mcp::CommandResponse::ERROR);
            error_response.set_message("Stream command failed: " + std::string(e.what()));
            
            if (!stream->Write(error_response)) {
                break;
            }
        }
    }
    
    return grpc::Status::OK;
}

grpc::Status GrpcMCPServer::RegisterAgent(grpc::ServerContext* context,
                                         const magda::mcp::RegisterAgentRequest* request,
                                         magda::mcp::RegisterAgentResponse* response) {
    try {
        std::string agent_id = generateAgentId();
        
        auto agent = std::make_shared<AgentConnection>(agent_id, request->agent_name(), request->agent_type());
        for (const auto& [key, value] : request->capabilities()) {
            agent->capabilities[key] = value;
        }
        
        addAgent(agent_id, agent);
        
        response->set_agent_id(agent_id);
        response->set_server_version("0.1.0");
        
        auto status = response->mutable_status();
        status->set_status(magda::mcp::CommandResponse::SUCCESS);
        status->set_message("Agent registered successfully");
        
        std::cout << "Agent registered: " << agent_id << " (" << request->agent_name() << ")" << std::endl;
        
        return grpc::Status::OK;
        
    } catch (const std::exception& e) {
        auto status = response->mutable_status();
        status->set_status(magda::mcp::CommandResponse::ERROR);
        status->set_message("Registration failed: " + std::string(e.what()));
        
        return grpc::Status::OK;
    }
}

grpc::Status GrpcMCPServer::SendMessage(grpc::ServerContext* context,
                                       const magda::mcp::SendMessageRequest* request,
                                       magda::mcp::SendMessageResponse* response) {
    try {
        sendToAgent(request->target_agent_id(), request->message());
        
        auto status = response->mutable_status();
        status->set_status(magda::mcp::CommandResponse::SUCCESS);
        status->set_message("Message sent");
        
        return grpc::Status::OK;
        
    } catch (const std::exception& e) {
        auto status = response->mutable_status();
        status->set_status(magda::mcp::CommandResponse::ERROR);
        status->set_message("Failed to send message: " + std::string(e.what()));
        
        return grpc::Status::OK;
    }
}

grpc::Status GrpcMCPServer::BroadcastMessage(grpc::ServerContext* context,
                                            const magda::mcp::BroadcastMessageRequest* request,
                                            magda::mcp::BroadcastMessageResponse* response) {
    try {
        broadcastMessage(request->message());
        
        auto status = response->mutable_status();
        status->set_status(magda::mcp::CommandResponse::SUCCESS);
        status->set_message("Message broadcast");
        
        response->set_recipients_count(static_cast<int32_t>(getAgentCount()));
        
        return grpc::Status::OK;
        
    } catch (const std::exception& e) {
        auto status = response->mutable_status();
        status->set_status(magda::mcp::CommandResponse::ERROR);
        status->set_message("Failed to broadcast message: " + std::string(e.what()));
        
        return grpc::Status::OK;
    }
}

grpc::Status GrpcMCPServer::GetConnectedAgents(grpc::ServerContext* context,
                                              const magda::mcp::GetConnectedAgentsRequest* request,
                                              magda::mcp::GetConnectedAgentsResponse* response) {
    std::lock_guard<std::mutex> lock(agents_mutex_);
    
    for (const auto& [agent_id, agent] : agents_) {
        auto agent_info = response->add_agents();
        agent_info->set_agent_id(agent->agent_id);
        agent_info->set_agent_name(agent->agent_name);
        agent_info->set_agent_type(agent->agent_type);
        agent_info->set_connected_timestamp(agent->connected_timestamp);
        
        for (const auto& [key, value] : agent->capabilities) {
            (*agent_info->mutable_capabilities())[key] = value;
        }
    }
    
    return grpc::Status::OK;
}

std::string GrpcMCPServer::generateAgentId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::stringstream ss;
    ss << "agent_" << dis(gen);
    return ss.str();
}

Command GrpcMCPServer::convertFromProto(const magda::mcp::CommandRequest& proto_cmd) {
    Command command(proto_cmd.command_type());
    
    for (const auto& [key, param_value] : proto_cmd.parameters()) {
        switch (param_value.value_case()) {
            case magda::mcp::ParameterValue::kStringValue:
                command.setParameter(key, param_value.string_value());
                break;
            case magda::mcp::ParameterValue::kIntValue:
                command.setParameter(key, param_value.int_value());
                break;
            case magda::mcp::ParameterValue::kDoubleValue:
                command.setParameter(key, param_value.double_value());
                break;
            case magda::mcp::ParameterValue::kBoolValue:
                command.setParameter(key, param_value.bool_value());
                break;
            case magda::mcp::ParameterValue::kDoubleArrayValue: {
                std::vector<double> values;
                for (double val : param_value.double_array_value().values()) {
                    values.push_back(val);
                }
                command.setParameter(key, values);
                break;
            }
            default:
                // Ignore unknown parameter types
                break;
        }
    }
    
    return command;
}

void GrpcMCPServer::convertToProto(const CommandResponse& cmd_response, magda::mcp::CommandResponse* proto_response) {
    switch (cmd_response.getStatus()) {
        case CommandResponse::Status::Success:
            proto_response->set_status(magda::mcp::CommandResponse::SUCCESS);
            break;
        case CommandResponse::Status::Error:
            proto_response->set_status(magda::mcp::CommandResponse::ERROR);
            break;
        case CommandResponse::Status::Pending:
            proto_response->set_status(magda::mcp::CommandResponse::PENDING);
            break;
    }
    
    proto_response->set_message(cmd_response.getMessage());
    
    if (!cmd_response.getData().empty()) {
        proto_response->set_data(cmd_response.getData().dump());
    }
}

CommandResponse GrpcMCPServer::executeCommand(const Command& command) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    
    auto it = command_handlers_.find(command.getType());
    if (it != command_handlers_.end()) {
        try {
            return it->second(command);
        } catch (const std::exception& e) {
            return CommandResponse(CommandResponse::Status::Error, 
                                 "Command execution failed: " + std::string(e.what()));
        }
    }
    
    return CommandResponse(CommandResponse::Status::Error, 
                         "Unknown command: " + command.getType());
}

void GrpcMCPServer::addAgent(const std::string& agent_id, std::shared_ptr<AgentConnection> agent) {
    std::lock_guard<std::mutex> lock(agents_mutex_);
    agents_[agent_id] = agent;
}

void GrpcMCPServer::removeAgent(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(agents_mutex_);
    agents_.erase(agent_id);
}

std::shared_ptr<GrpcMCPServer::AgentConnection> GrpcMCPServer::getAgent(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(agents_mutex_);
    auto it = agents_.find(agent_id);
    return (it != agents_.end()) ? it->second : nullptr;
} 