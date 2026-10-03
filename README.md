# FrostSoulX Audio Engine

Canonical C++17 native DSP source for FrostSoulX. There is one processing path,
no runtime backend selector, and no Steam Audio library dependency:

```text
float PCM → orthonormal M/S → independent bass/high-band processing
          → FrostSoulX spatial model (geometry + ILD/ITD/HRTF)
          → full non-uniform 2×2 partitioned convolution
          → linked true-peak/lookahead safety → float PCM
```

The linear spatial model is compiled into four source-to-ear BRIR filters on
one control thread. Direct HRTF, room reflections and internal first-order
spherical-harmonic diffuse-field modelling are fused into that matrix: there
is no second HOA renderer or duplicate runtime HRTF convolution.

```cpp
#include "frostsoulx/immersive_audio_engine.h"
frostsoulx::ImmersiveAudioEngine engine;
engine.setRoomSimulationPreset(frostsoulx::RoomSimulationPreset::Off);
engine.setBassGain(1.0f);
engine.setBassWidth(1.0f);
engine.setHighBandWidth(1.0f);
engine.setSpatialBlend(1.0f); // 0 is delay-matched dry
engine.prepare(48000, 384);
engine.setEnabled(true);
engine.process(interleavedStereo, frames);
```

## Runtime and gain contract

- Recommended callback remains **384 frames**. A persistent 128-frame adapter
  accepts partial callbacks without padding, advancing time, or losing output.
- Orthonormal M/S and the complementary `high = input - low` split reconstruct
  unity to float precision. Bass gain/width and high-band width are independent,
  sample-smoothed controls; boosts consume explicitly bounded headroom.
  Legacy normalized `stereo_width=0.5` maps to high-band unity.
- Complete transfer rows have `sum_source sum_tap abs(h) <= 0.98`. Normalization
  attenuates only and preserves relative ear gains, avoiding forced IR boosts.
- Correlated IR/dry-wet transitions use **unity-sum** fades. All convolution
  tiers transition on the same sample timeline with retained input history.
- One forward FFT per input/tier and one inverse FFT per output/tier; empty IR
  work is skipped. Buffers and FFT state are reused. ARM64 NEON/x86 SIMD and
  scoped denormal control are supported.
- The final safety stage is a 4×, 64-tap reconstruction detector with 64 samples
  of lookahead and linked gain, not a soft clipper. Its detection margin targets
  0.95 beneath the 0.98 output ceiling. This is not a BS.1770 certification claim.
- `process()` performs no heap allocation/deallocation, locking, I/O or logging.
  One control producer may publish whole IR matrices concurrently through a
  bounded atomic mailbox; rapid updates coalesce while a fade completes.
- `prepare()`, `reset()` and destruction must be serialized with both threads.
  Profile/matrix getters belong to the control thread; `safetyGain()` is an
  audio-thread diagnostic. Scalar parameter targets are lock-free atomics.
- Reported latency includes block adaptation, nominal HRTF padding and safety:
  **210 samples at 48 kHz**. Direction-dependent acoustic ear delays remain in
  the wet filters; the dry branch uses the nominal common HRTF pad.

## Build and verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFROSTSOULX_BUILD_PLUGIN=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/frostsoulx_unified_dsp_tests
```

Native suites cover unity/band independence, all four convolution transfers
against direct references, tier edges, partial callbacks, gain and transition
bounds, NaN/Inf, concurrent IR publication, callback heap instrumentation and
independent 16× reconstructed true peaks. Release timing gates are not applied
to sanitizer-instrumented runs. Android ARM64-v8a/x86_64 builds run in CI.

The six-function float-PCM plugin ABI is retained. The C++ backend enum,
backend preference/getter and old backend-specific result codes are removed.
FrostSoulX's external JNI/Media3 adapter, integer PCM conversion, application
UI and strict renderer OFF bypass are **not in this repository**. Those hosts
must migrate removed C++ calls, honor latency, serialize lifecycle operations,
and perform saturating, NaN-safe integer conversion; they cannot be validated
by these native tests. Disabled processing leaves the caller's buffer intact.
