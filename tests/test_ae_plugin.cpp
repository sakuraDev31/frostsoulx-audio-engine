#include "ae_plugin.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << "\n";
        ++failures;
    }
}

bool sane(const std::vector<float>& data) {
    for (float sample : data) {
        if (!std::isfinite(sample) || std::fabs(sample) > 0.99f) return false;
    }
    return true;
}
}

int main() {
    check(ae_abi_version() == AE_ABI_VERSION, "ABI version is 1");
    check(ae_create(48000, 1) == nullptr, "mono creation is rejected");
    check(ae_create(4000, 2) == nullptr, "unsupported sample rate is rejected");

    AeEngine* engine = ae_create(48000, 2);
    check(engine != nullptr, "stereo engine is created");
    if (!engine) return 1;

    for (const auto& param : {
             std::pair<const char*, float>{"enabled", 1.0f},
             {"spatial_blend", 0.8f},
             {"room_preset", 3.0f},
             {"room_mix", 0.2f},
             {"reflection_amount", 0.35f},
             {"reverb_time", 1.7f},
             {"room_size", 0.6f},
             {"dampening", 0.4f},
             {"stereo_width", 0.9f},
             {"head_yaw", 12.0f},
             {"head_pitch", -4.0f},
             {"head_roll", 2.0f},
         }) {
        ae_set_param(engine, param.first, param.second);
    }
    ae_set_param(engine, "unknown_parameter", 123.0f);
    ae_set_param(engine, "room_mix", std::numeric_limits<float>::quiet_NaN());

    for (int frames : {1, 7, 128, 384, 1024}) {
        std::vector<float> data(static_cast<std::size_t>(frames) * 2);
        for (int i = 0; i < frames; ++i) {
            data[static_cast<std::size_t>(i) * 2] = 0.35f * std::sin(i * 0.021f);
            data[static_cast<std::size_t>(i) * 2 + 1] = 0.25f * std::cos(i * 0.017f);
        }
        ae_process(engine, data.data(), frames);
        check(sane(data), "processed output is finite and bounded for block size " + std::to_string(frames));
    }

    std::vector<float> bypass(768, 0.25f);
    const auto original = bypass;
    ae_set_param(engine, "enabled", 0.0f);
    ae_process(engine, bypass.data(), 384);
    check(bypass == original, "disabled ABI path is bit-transparent");

    ae_reset(engine);
    ae_set_param(engine, "enabled", 1.0f);
    ae_process(engine, bypass.data(), 384);
    check(sane(bypass), "output after reset is finite and bounded");

    ae_destroy(engine);
    std::cout << (failures == 0 ? "ABI plugin checks passed\n" : "ABI plugin checks failed\n");
    return failures == 0 ? 0 : 1;
}
