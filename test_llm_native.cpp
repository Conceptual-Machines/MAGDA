#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// llama.cpp headers
#include "llama.h"

class NativeLLMModel {
  private:
    llama_model* model_;
    llama_context* context_;
    std::string model_path_;
    std::string model_name_;
    bool loaded_;

  public:
    NativeLLMModel(const std::string& name, const std::string& path)
        : model_(nullptr),
          context_(nullptr),
          model_path_(path),
          model_name_(name),
          loaded_(false) {}

    ~NativeLLMModel() {
        if (context_) {
            llama_free(context_);
        }
        if (model_) {
            llama_free_model(model_);
        }
    }

    bool load() {
        std::cout << "🔄 Loading model " << model_name_ << ": " << model_path_ << std::endl;

        // Initialize llama.cpp backend
        llama_backend_init();

        // Set up model parameters
        llama_model_params model_params = llama_model_default_params();
        model_params.n_gpu_layers = 0;  // CPU only for now, can enable GPU later
        model_params.use_mmap = true;
        model_params.use_mlock = false;

        // Load model
        model_ = llama_load_model_from_file(model_path_.c_str(), model_params);
        if (!model_) {
            std::cerr << "❌ Failed to load model from " << model_path_ << std::endl;
            return false;
        }

        // Set up context parameters
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.seed = 42;
        ctx_params.n_ctx = 2048;   // Context window
        ctx_params.n_threads = 4;  // Number of threads
        ctx_params.n_threads_batch = 4;

        // Create context
        context_ = llama_new_context_with_model(model_, ctx_params);
        if (!context_) {
            std::cerr << "❌ Failed to create context" << std::endl;
            llama_free_model(model_);
            model_ = nullptr;
            return false;
        }

        loaded_ = true;
        std::cout << "✅ Model " << model_name_ << " loaded successfully" << std::endl;
        return true;
    }

    std::string generate(const std::string& prompt, int max_tokens = 20) {
        if (!loaded_ || !context_) {
            return "ERROR: Model not loaded";
        }

        auto start = std::chrono::high_resolution_clock::now();

        // Tokenize prompt
        std::vector<llama_token> tokens;
        tokens.resize(prompt.length() + 128);  // Extra space for tokenization
        int n_tokens = llama_tokenize(model_, prompt.c_str(), prompt.length(), tokens.data(),
                                      tokens.size(), true, true);

        if (n_tokens < 0) {
            std::cerr << "❌ Failed to tokenize prompt" << std::endl;
            return "ERROR: Tokenization failed";
        }

        tokens.resize(n_tokens);

        std::cout << "🔍 Tokenized prompt: " << n_tokens << " tokens" << std::endl;

        // Clear context
        llama_kv_cache_clear(context_);

        // Decode prompt tokens
        llama_batch batch = llama_batch_init(n_tokens, 0, 1);
        for (int i = 0; i < n_tokens; ++i) {
            llama_batch_add(batch, tokens[i], i, {0}, false);
        }
        batch.logits[batch.n_tokens - 1] = true;  // Only need logits for last token

        if (llama_decode(context_, batch) != 0) {
            std::cerr << "❌ Failed to decode prompt" << std::endl;
            llama_batch_free(batch);
            return "ERROR: Decode failed";
        }

        // Generate tokens
        std::string result;
        int n_generated = 0;

        for (int i = 0; i < max_tokens; ++i) {
            // Sample next token
            auto* logits = llama_get_logits_ith(context_, batch.n_tokens - 1);
            auto n_vocab = llama_n_vocab(model_);

            std::vector<llama_token_data> candidates;
            candidates.reserve(n_vocab);

            for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
                candidates.emplace_back(llama_token_data{token_id, logits[token_id], 0.0f});
            }

            llama_token_data_array candidates_p = {candidates.data(), candidates.size(), false};

            // Sample with temperature
            llama_sample_temp(context_, &candidates_p, 0.1f);
            llama_token new_token = llama_sample_token(context_, &candidates_p);

            // Check for end-of-sequence
            if (llama_token_is_eog(model_, new_token)) {
                break;
            }

            // Convert token to text
            char token_text[32];
            int len =
                llama_token_to_piece(model_, new_token, token_text, sizeof(token_text), false);
            if (len > 0) {
                result.append(token_text, len);
            }

            // Prepare for next iteration
            llama_batch_clear(batch);
            llama_batch_add(batch, new_token, n_tokens + i, {0}, true);

            if (llama_decode(context_, batch) != 0) {
                std::cerr << "❌ Failed to decode generated token" << std::endl;
                break;
            }

            n_generated++;
        }

        llama_batch_free(batch);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "⚡ Generated " << n_generated << " tokens in " << duration.count() << "ms"
                  << std::endl;

        return result;
    }

    bool isLoaded() const {
        return loaded_;
    }

    std::string getName() const {
        return model_name_;
    }
};

int main() {
    std::cout << "🧪 Native LLM Integration Test" << std::endl;
    std::cout << "==============================" << std::endl;

    // Test the 8B model with native API
    NativeLLMModel model(
        "llama31-8b", "/Volumes/External SSD/MAGICA/models/Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf");

    if (!model.load()) {
        std::cerr << "❌ Failed to load model" << std::endl;
        return 1;
    }

    // Test 1: Simple function call generation
    std::cout << "\n🧪 Test 1: Simple response" << std::endl;
    std::cout << "=========================" << std::endl;

    std::string prompt1 = "Hello";
    std::string result1 = model.generate(prompt1, 10);
    std::cout << "Prompt: " << prompt1 << std::endl;
    std::cout << "Result: '" << result1 << "'" << std::endl;

    // Test 2: Function call generation
    std::cout << "\n🧪 Test 2: Function call generation" << std::endl;
    std::cout << "===================================" << std::endl;

    std::string prompt2 = "Generate: track(\"guitar\")";
    std::string result2 = model.generate(prompt2, 15);
    std::cout << "Prompt: " << prompt2 << std::endl;
    std::cout << "Result: '" << result2 << "'" << std::endl;

    // Test 3: Sequential command parsing
    std::cout << "\n🧪 Test 3: Command parsing" << std::endl;
    std::cout << "=========================" << std::endl;

    std::string prompt3 = "Parse: create track and clip";
    std::string result3 = model.generate(prompt3, 20);
    std::cout << "Prompt: " << prompt3 << std::endl;
    std::cout << "Result: '" << result3 << "'" << std::endl;

    std::cout << "\n✅ Native LLM test completed" << std::endl;
    return 0;
}
