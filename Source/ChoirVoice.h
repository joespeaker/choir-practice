#pragma once

#include <juce_dsp/juce_dsp.h>

/**
    A single "singer" in the choir.

    Each ChoirVoice pushes the incoming signal through its own fractionally
    modulated delay line. Slowly wobbling the delay time is what gives the
    voice its natural, slightly-detuned pitch drift (the same trick used by
    classic doubler/chorus effects) without the artifacts of a true
    phase-vocoder pitch shifter. A gentle peak filter gives each voice a
    slightly different timbre, and it's panned to its own slot in the
    stereo field.

    The random "personality" of a voice (its pan slot jitter, LFO phase,
    LFO rate jitter, delay-time jitter and tone colour) is generated once in
    prepare() from a seed derived from the voice index, so a given voice
    always sounds like the same singer across a session even as the knobs
    move. updateBlockParams() should be called once per audio block with the
    current parameter values; processSample() is then called once per sample.
*/
class ChoirVoice
{
public:
    ChoirVoice() = default;

    void prepare (const juce::dsp::ProcessSpec& spec, int voiceIndex);
    void reset();

    /** Recomputes the per-block control values from the current knob settings.
        Call once per block, before any processSample() calls, for every voice
        that is currently active (index < number of voices in use).
    */
    void updateBlockParams (int voiceIndex,
                             int numVoicesInUse,
                             float detuneAmount01,
                             float movementAmount01,
                             float widthAmount01,
                             float spreadAmount01);

    /** Processes one input sample and adds this voice's contribution into
        the stereo accumulator outL/outR. Gain compensation for the number
        of active voices is already applied.
    */
    void processSample (float inputSample, float& outL, float& outR);

private:
    static float panSlotFor (int voiceIndex, int numVoicesInUse);

    double sampleRate = 44100.0;

    // Stable per-voice "personality", generated once in prepare().
    float panJitter = 0.0f;
    float rateJitterMultiplier = 1.0f;
    float phaseStartRadians = 0.0f;
    float delayJitterMs = 0.0f;
    float toneFrequencyHz = 6000.0f;
    float toneGainDb = 0.0f;

    // Runtime state.
    float phase = 0.0f;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { 8192 };
    juce::dsp::IIR::Filter<float> toneFilter;

    // Cached per-block control values, refreshed by updateBlockParams().
    float pan = 0.0f;
    float baseDelaySamples = 0.0f;
    float modulationDepthSamples = 0.0f;
    float phaseIncrement = 0.0f;
    float gainCompensation = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoirVoice)
};
