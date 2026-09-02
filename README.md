# Choir Practice

An Audio Unit / VST3 plugin that turns a single vocal into a choir. Built with
[JUCE](https://juce.com), installs into Logic Pro (and any other AU/VST3 host)
on macOS.

## How it works

Rather than a true pitch-shifter, each of up to 8 "voices" runs the input
through its own modulated fractional delay line — slowly wobbling the delay
time is what gives a voice its natural, slightly-detuned pitch drift (the
same trick classic doubler/chorus effects use). Every voice has its own
randomized LFO phase and rate, a small tone-colour variation, and its own
spot in the stereo field, so the voices don't sound phase-locked to each
other. The blend is then sent through a reverb for choir-in-a-room ambience.

Controls:

| Knob | What it does |
| --- | --- |
| Voices | Number of choir voices (1–8) |
| Detune | How far each voice's pitch drifts from center |
| Movement | How fast that drift wanders |
| Width | Stereo spread of the voices |
| Spread | How far apart the voices sit in time (doubling amount) |
| Mix | Dry/wet blend of the effect |
| Reverb | Amount of reverb send |
| Size | Reverb room size |

## Requirements

- macOS with Xcode (command line tools are enough) and CMake 3.22+
- Internet access on first configure (CMake fetches JUCE via `FetchContent`)

## Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

`COPY_PLUGIN_AFTER_BUILD` is enabled, so a successful build automatically
copies:

- the AU component to `~/Library/Audio/Plug-Ins/Components/Choir Practice.component`
- the VST3 to `~/Library/Audio/Plug-Ins/VST3/Choir Practice.vst3`
- a standalone app to `build/ChoirPractice_artefacts/Release/Standalone`

## Validate the AU (optional but recommended)

```sh
auval -v aufx Chrp Jspk
```

If `auval` reports `PASS`, the component is well-formed.

## Use it in Logic Pro

1. Quit and reopen Logic Pro (or run **Logic Pro → Preferences → Audio →
   Plug-In Manager** and rescan) so it picks up the new plugin.
2. Insert **Choir Practice** on a vocal track (mono or stereo input both
   work; output is always stereo).
3. Start around the defaults (5 voices, Detune 35%, Width 85%) and adjust
   from there — more voices and width read as a bigger choir, more detune
   and movement read as a looser, more human blend.

## Note on this build

This project was scaffolded and the DSP/UI code written in a Linux
container without Xcode or the AudioUnit SDK, so it hasn't been compiled or
`auval`-validated yet. Build it on your Mac first and let me know what, if
anything, doesn't compile — CMake/JUCE error messages are usually specific
enough to fix quickly.
