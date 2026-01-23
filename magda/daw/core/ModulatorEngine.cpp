#include "ModulatorEngine.hpp"

#include "TrackManager.hpp"

namespace magda {

void ModulatorEngine::updateAllMods(double deltaTime) {
    // Delegate to TrackManager which has access to all tracks and chains
    TrackManager::getInstance().updateAllMods(deltaTime);
}

void ModulatorEngine::timerCallback() {
    constexpr double deltaTime = 0.016;  // ~60 FPS
    updateAllMods(deltaTime);

    // Trigger UI refresh for parameter indicators
    TrackManager::getInstance().notifyModulationChanged();
}

}  // namespace magda
