#include "ae_plugin.h"

#include "frostsoulx/immersive_audio_engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

struct AeEngine {
    frostsoulx::ImmersiveAudioEngine engine;
    int maxFrames = 0;
    float headYaw = 0.0f;
    float headPitch = 0.0f;
    float headRoll = 0.0f;
};

namespace {

constexpr int kMaxPluginFrames = 8192;

int roomPresetFromValue(float value) noexcept {
    if (!std::isfinite(value)) return 0;
    const int choice = static_cast<int>(std::lround(value));
    return std::clamp(choice, 0, 5);
}

frostsoulx::RoomSimulationPreset roomPreset(int choice) noexcept {
    switch (choice) {
        case 1: return frostsoulx::RoomSimulationPreset::SmallRoom;
        case 2: return frostsoulx::RoomSimulationPreset::Studio;
        case 3: return frostsoulx::RoomSimulationPreset::ConcertHall;
        case 4: return frostsoulx::RoomSimulationPreset::Cathedral;
        case 5: return frostsoulx::RoomSimulationPreset::Subway;
        case 0:
        default: return frostsoulx::RoomSimulationPreset::Off;
    }
}

} // namespace

extern "C" {

AE_EXPORT int ae_abi_version(void) {
    return AE_ABI_VERSION;
}

AE_EXPORT AeEngine* ae_create(int sample_rate, int channels) {
    if (channels != 2 || sample_rate < 8000 || sample_rate > 384000) return nullptr;

    auto* e = new AeEngine();
    e->maxFrames = kMaxPluginFrames;
    if (!e->engine.prepare(sample_rate, e->maxFrames)) {
        delete e;
        return nullptr;
    }

    // The plugin starts as a transparent native spatial engine. The host can
    // then apply manifest parameters before the first audio callback.
    e->engine.setRoomSimulationPreset(frostsoulx::RoomSimulationPreset::Off);
    e->engine.setRoomMix(0.0f);
    e->engine.setSpatialBlend(1.0f);
    e->engine.setEnabled(true);
    return e;
}

AE_EXPORT void ae_destroy(AeEngine* e) {
    delete e;
}

AE_EXPORT void ae_set_param(AeEngine* e, const char* id, float value) {
    if (!e || !id || !std::isfinite(value)) return;

    if (std::strcmp(id, "enabled") == 0) {
        e->engine.setEnabled(value >= 0.5f);
    } else if (std::strcmp(id, "spatial_blend") == 0) {
        e->engine.setSpatialBlend(value);
    } else if (std::strcmp(id, "room_preset") == 0) {
        e->engine.setRoomSimulationPreset(roomPreset(roomPresetFromValue(value)));
    } else if (std::strcmp(id, "room_mix") == 0) {
        e->engine.setRoomMix(value);
    } else if (std::strcmp(id, "reflection_amount") == 0) {
        e->engine.setReflectionAmount(value);
    } else if (std::strcmp(id, "reverb_time") == 0) {
        e->engine.setReverbTimeSeconds(value);
    } else if (std::strcmp(id, "room_size") == 0) {
        e->engine.setRoomSize(value);
    } else if (std::strcmp(id, "dampening") == 0) {
        e->engine.setDampening(value);
    } else if (std::strcmp(id, "stereo_width") == 0) {
        e->engine.setStereoWidth(value);
    } else if (std::strcmp(id, "head_yaw") == 0) {
        e->headYaw = value;
        e->engine.setHeadOrientation(e->headYaw, e->headPitch, e->headRoll);
    } else if (std::strcmp(id, "head_pitch") == 0) {
        e->headPitch = value;
        e->engine.setHeadOrientation(e->headYaw, e->headPitch, e->headRoll);
    } else if (std::strcmp(id, "head_roll") == 0) {
        e->headRoll = value;
        e->engine.setHeadOrientation(e->headYaw, e->headPitch, e->headRoll);
    }
    // Unknown parameter IDs are intentionally ignored per ABI contract.
}

AE_EXPORT void ae_process(AeEngine* e, float* data, int frames) {
    if (!e || !data || frames <= 0 || frames > e->maxFrames) return;
    (void)e->engine.process(data, frames);
}

AE_EXPORT void ae_reset(AeEngine* e) {
    if (e) e->engine.reset();
}

} // extern "C"
