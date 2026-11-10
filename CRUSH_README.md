# CRUSH – mid saturation / crush VST3

Saturation plugin with 3 modes, 5 source voicings and a big drive dial.

- **DRIVE**: how hard you hit the saturator (0-100%)
- **SOFT / HARD / HIT**: soft tanh, near-hard clip, asymmetric + bit-crush
- **SOURCE**: KICK / SNARE / 808 / HI-HATS / VOCALS (pre-saturation EQ push per source)
- **MID**: extra mid push before saturation
- **MIX / OUT**, **SAFETY** soft limiter on the output
- 2x oversampling, latency reported to the host

## Option A – no install, build online (easiest)

1. Make a free account at github.com and create a new repository.
2. Upload this whole folder (including the hidden `.github` folder) to it.
3. Open the **Actions** tab. The "Build Crush VST3" job runs by itself (about 10 min).
4. Open the finished run and download **Crush-VST3-Windows** under Artifacts. Unzip it.

## Option B – build on your PC

Install: Visual Studio 2022 (workload "Desktop development with C++"), CMake and Git.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The plugin is at `build/Crush_artefacts/Release/VST3/Crush.vst3`.

## Install in FL Studio

1. Copy the whole `Crush.vst3` folder to `C:\Program Files\Common Files\VST3\`
2. In FL Studio: **Options > Manage plugins > Find installed plugins** (start scan).
3. Add it from the plugin database (Effects > Installed > Crush) on a mixer insert.

Note: this is untested code. If the build prints an error, send it to Claude and it will be fixed.
