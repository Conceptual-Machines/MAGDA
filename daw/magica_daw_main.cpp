#include <memory>
#include <thread>
#include <iostream>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <grpcpp/grpcpp.h>
#include "../mcp/proto/magica_daw.grpc.pb.h"

#include "engine/tracktion_engine.hpp"
#include "../mcp/server/magica_daw_server.hpp"
#include "ui/main_window.hpp"

using grpc::Server;
using grpc::ServerBuilder;

class MagicaDAWApplication : public juce::JUCEApplication {
private:
    std::unique_ptr<TracktionEngine> daw_engine_;
    std::unique_ptr<MagicaDAWServer> grpc_server_;
    std::unique_ptr<MainWindow> main_window_;
    std::unique_ptr<Server> grpc_server_instance_;
    std::thread grpc_thread_;
    
public:
        MagicaDAWApplication() = default;

    const juce::String getApplicationName() override { return "Magica DAW"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    
    void initialise(const juce::String& commandLine) override {
        // 1. Initialize audio engine
        daw_engine_ = std::make_unique<TracktionEngine>();
        daw_engine_->initialize();
        
        std::cout << "✓ Audio engine initialized" << std::endl;
        
        // 2. Create gRPC server with DAW engine
        grpc_server_ = std::make_unique<MagicaDAWServer>(daw_engine_.get());
        
        // 3. Start gRPC server in background thread
        grpc_thread_ = std::thread([this]() {
            std::string server_address("0.0.0.0:50051");
            
            ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(grpc_server_.get());
            
            grpc_server_instance_ = builder.BuildAndStart();
            std::cout << "🚀 Magica DAW gRPC server listening on " << server_address << std::endl;
            std::cout << "🤖 Ready for AI agents to connect!" << std::endl;
            
            // Keep server running
            grpc_server_instance_->Wait();
        });
        
        // 4. Create main UI window
        main_window_ = std::make_unique<MainWindow>(daw_engine_.get(), grpc_server_.get());
        main_window_->setVisible(true);
        
        std::cout << "🎵 Magica DAW is ready!" << std::endl;
        std::cout << "" << std::endl;
        std::cout << "Connect agents with:" << std::endl;
        std::cout << "  python agents/orchestrator.py --daw localhost:50051" << std::endl;
        std::cout << "  go run agents/utility_agent.go --daw localhost:50051" << std::endl;
    }
    
    void shutdown() override {
        // Graceful shutdown
        if (grpc_server_instance_) {
            grpc_server_instance_->Shutdown();
        }
        if (grpc_thread_.joinable()) {
            grpc_thread_.join();
        }
        
        main_window_.reset();
        grpc_server_.reset();
        daw_engine_.reset();
        
        std::cout << "👋 Magica DAW shutdown complete" << std::endl;
    }
    
    void systemRequestedQuit() override {
        quit();
    }
};

// JUCE application startup
START_JUCE_APPLICATION(MagicaDAWApplication) 