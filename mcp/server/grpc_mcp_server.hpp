#pragma once

#include <grpcpp/grpcpp.h>
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include "mcp_server_interface.hpp"
#include "command.hpp"
#include "mcp_service.grpc.pb.h"

/**
 * @brief gRPC-based Multi-agent Control Protocol Server
 * 
 * The GrpcMCPServer provides a gRPC interface for agent communication,
 * command routing, and real-time bidirectional streaming between agents
 * and the DAW.
 */
class GrpcMCPServer : public MCPServerInterface, public magda::mcp::MCPService::Service {
public:
    /**
     * @brief Construct gRPC MCP server
     * @param port Port to listen on
     */
    explicit GrpcMCPServer(int port = 50051);
    
    /**
     * @brief Destructor
     */
    ~GrpcMCPServer() override;
    
    // MCPServerInterface implementation
    bool start() override;
    void stop() override;
    bool isRunning() const override { return running_.load(); }
    void registerCommandHandler(const std::string& command_type, CommandHandler handler) override;
    void broadcastMessage(const std::string& message) override;
    void sendToAgent(const std::string& agent_id, const std::string& message) override;
    std::vector<std::string> getConnectedAgents() const override;
    size_t getAgentCount() const override;
    std::string getServerType() const override { return "gRPC"; }
    int getPort() const override { return port_; }
    
    // gRPC service implementation
    grpc::Status ExecuteCommand(grpc::ServerContext* context,
                               const magda::mcp::CommandRequest* request,
                               magda::mcp::CommandResponse* response) override;
    
    grpc::Status CommandStream(grpc::ServerContext* context,
                              grpc::ServerReaderWriter<magda::mcp::CommandResponse,
                                                      magda::mcp::CommandRequest>* stream) override;
    
    grpc::Status RegisterAgent(grpc::ServerContext* context,
                              const magda::mcp::RegisterAgentRequest* request,
                              magda::mcp::RegisterAgentResponse* response) override;
    
    grpc::Status SendMessage(grpc::ServerContext* context,
                            const magda::mcp::SendMessageRequest* request,
                            magda::mcp::SendMessageResponse* response) override;
    
    grpc::Status BroadcastMessage(grpc::ServerContext* context,
                                 const magda::mcp::BroadcastMessageRequest* request,
                                 magda::mcp::BroadcastMessageResponse* response) override;
    
    grpc::Status GetConnectedAgents(grpc::ServerContext* context,
                                   const magda::mcp::GetConnectedAgentsRequest* request,
                                   magda::mcp::GetConnectedAgentsResponse* response) override;

private:
    struct AgentConnection {
        std::string agent_id;
        std::string agent_name;
        std::string agent_type;
        std::map<std::string, std::string> capabilities;
        int64_t connected_timestamp;
        grpc::ServerReaderWriter<magda::mcp::CommandResponse, magda::mcp::CommandRequest>* stream;
        
        AgentConnection(const std::string& id, const std::string& name, const std::string& type)
            : agent_id(id), agent_name(name), agent_type(type), 
              connected_timestamp(std::time(nullptr)), stream(nullptr) {}
    };
    
    // Helper methods
    std::string generateAgentId();
    Command convertFromProto(const magda::mcp::CommandRequest& proto_cmd);
    void convertToProto(const CommandResponse& cmd_response, magda::mcp::CommandResponse* proto_response);
    CommandResponse executeCommand(const Command& command);
    
    // Agent management
    void addAgent(const std::string& agent_id, std::shared_ptr<AgentConnection> agent);
    void removeAgent(const std::string& agent_id);
    std::shared_ptr<AgentConnection> getAgent(const std::string& agent_id);
    
    std::unique_ptr<grpc::Server> server_;
    std::atomic<bool> running_;
    int port_;
    
    // Agent tracking
    mutable std::mutex agents_mutex_;
    std::map<std::string, std::shared_ptr<AgentConnection>> agents_;
    
    // Command handlers
    mutable std::mutex handlers_mutex_;
    std::map<std::string, CommandHandler> command_handlers_;
};

// For backward compatibility, alias the old name
using MCPServer = GrpcMCPServer; 