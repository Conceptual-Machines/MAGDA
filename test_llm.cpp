#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>

class LLMModel {
  private:
    std::string model_path_;
    std::string model_name_;
    bool loaded_;

  public:
    LLMModel(const std::string& name, const std::string& path)
        : model_path_(path), model_name_(name), loaded_(false) {}

    bool load() {
        std::cout << "🔄 Loading model " << model_name_ << ": " << model_path_ << std::endl;
        loaded_ = true;
        std::cout << "✅ Model " << model_name_ << " loaded successfully" << std::endl;
        return true;
    }

    std::string generate(const std::string& prompt,
                         const std::map<std::string, std::string>& params) {
        if (!loaded_) {
            return "ERROR: Model not loaded";
        }

        auto start = std::chrono::high_resolution_clock::now();

        // Write prompt to temp file
        std::string temp_file = "temp_" + model_name_ + "_" + std::to_string(getpid()) + ".txt";
        std::ofstream file(temp_file);
        file << prompt;
        file.close();

        // Build command with parameters - properly quote paths with spaces
        std::string command = "timeout 30 /opt/homebrew/bin/llama-cli --model \"" + model_path_ +
                              "\" --file \"" + temp_file + "\"";

        // Add parameters
        for (const auto& [key, value] : params) {
            command += " --" + key + " " + value;
        }

        command += " --no-warmup";

        std::cout << "🔧 Executing: " << command << std::endl;

        // Execute
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            unlink(temp_file.c_str());
            return "ERROR: Failed to run LLM";
        }

        std::string result;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            result += buffer;
        }
        int exit_code = pclose(pipe);
        unlink(temp_file.c_str());

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "🔍 Raw LLM output (" << result.length() << " chars): '" << result << "'"
                  << std::endl;
        std::cout << "🔍 Exit code: " << exit_code << std::endl;
        std::cout << "⚡ " << model_name_ << " generated in " << duration.count() << "ms"
                  << std::endl;

        return result;
    }

    std::string getName() const {
        return model_name_;
    }

    bool isLoaded() const {
        return loaded_;
    }
};

int main() {
    std::cout << "🧪 LLM Integration Test" << std::endl;
    std::cout << "======================" << std::endl;

    // Test the 8B model
    LLMModel model("llama31-8b",
                   "/Volumes/External SSD/MAGDA/models/Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf");

    if (!model.load()) {
        std::cerr << "❌ Failed to load model" << std::endl;
        return 1;
    }

    // Test parameters
    std::map<std::string, std::string> params;
    params["temp"] = "0.01";
    params["n-predict"] = "20";
    params["seed"] = "42";

    // Test 1: Simple function call generation
    std::cout << "\n🧪 Test 1: Simple function call generation" << std::endl;
    std::cout << "===========================================" << std::endl;

    std::string prompt1 = "Generate a function call: track(\"guitar\")";
    std::string result1 = model.generate(prompt1, params);
    std::cout << "Result: " << result1 << std::endl;

    // Test 2: Routing task
    std::cout << "\n🧪 Test 2: Sequential command detection" << std::endl;
    std::cout << "=======================================" << std::endl;

    std::string prompt2 = "Parse this command into a sequence: \"create track and clip\"\nRespond "
                          "with JSON format: [{\"type\":\"create_track\", \"target\":\"...\"}, "
                          "{\"type\":\"add_clip\", \"target\":\"...\", \"depends_on\":0}]";
    std::string result2 = model.generate(prompt2, params);
    std::cout << "Result: " << result2 << std::endl;

    // Test 3: Very simple task
    std::cout << "\n🧪 Test 3: Very simple response" << std::endl;
    std::cout << "===============================" << std::endl;

    std::string prompt3 = "Say hello";
    std::string result3 = model.generate(prompt3, params);
    std::cout << "Result: " << result3 << std::endl;

    return 0;
}
