#include "ModulatorEngine.hpp"

#include "TrackManager.hpp"

namespace magda {

void ModulatorEngine::updateAllMods(double deltaTime) {
    // Delegate to TrackManager to update all mods in all racks
    TrackManager::getInstance().updateAllMods(deltaTime);
}

}  // namespace magda
