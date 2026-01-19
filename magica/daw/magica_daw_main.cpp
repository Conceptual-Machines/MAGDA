#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <iostream>
#include <memory>

#include "engine/TracktionEngineWrapper.hpp"
#include "ui/themes/DarkTheme.hpp"
#include "ui/themes/FontManager.hpp"
#include "ui/windows/MainWindow.hpp"

using namespace juce;

class MagicaDAWApplication : public JUCEApplication {
  private:
    std::unique_ptr<magica::TracktionEngineWrapper> daw_engine_;
    std::unique_ptr<magica::MainWindow> mainWindow_;
    std::unique_ptr<juce::LookAndFeel> lookAndFeel_;

  public:
    MagicaDAWApplication() = default;

    const String getApplicationName() override {
        return "Magica DAW";
    }
    const String getApplicationVersion() override {
        return "1.0.0";
    }

    void initialise(const String& commandLine) override {
        // 1. Initialize fonts
        magica::FontManager::getInstance().initialize();

        // 2. Set up dark theme
        lookAndFeel_ = std::make_unique<juce::LookAndFeel_V4>();
        magica::DarkTheme::applyToLookAndFeel(*lookAndFeel_);
        juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel_.get());

        // 3. Initialize audio engine
        daw_engine_ = std::make_unique<magica::TracktionEngineWrapper>();
        if (!daw_engine_->initialize()) {
            std::cerr << "ERROR: Failed to initialize Tracktion Engine" << std::endl;
            quit();
            return;
        }

        std::cout << "✓ Audio engine initialized" << std::endl;

        // 4. Create main window with full UI
        mainWindow_ = std::make_unique<magica::MainWindow>();

        std::cout << "🎵 Magica DAW is ready!" << std::endl;
    }

    void shutdown() override {
        // Graceful shutdown
        mainWindow_.reset();
        daw_engine_.reset();

        // Reset look and feel
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
        lookAndFeel_.reset();

        std::cout << "👋 Magica DAW shutdown complete" << std::endl;
    }

    void systemRequestedQuit() override {
        quit();
    }
};

// JUCE application startup
START_JUCE_APPLICATION(MagicaDAWApplication)
