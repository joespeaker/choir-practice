#include "ChoirVoice.h"

namespace
{
    // Maximum modulation depth applied to the delay line, in milliseconds.
    // Kept small so the effect reads as "pitch drift" rather than vibrato.
    constexpr float maxDetuneDepthMs = 4.0f;

    // Range of the slowly-wandering LFO that drives each voice's delay time.
    constexpr float minMovementHz = 0.08f;
    constexpr float maxMovementHz = 1.6f;

    // Range of the static per-voice doubling delay, in milliseconds.
    constexpr float minSpreadDelayMs = 4.0f;
    constexpr float maxSpreadDelayMs = 30.0f;
}

float ChoirVoice::panSlotFor (int voiceIndex, int numVoicesInUse)
{
    if (numVoicesInUse <= 1)
        return 0.0f;

    // Evenly distribute voices from -1 (left) to +1 (right) by index.
    return juce::jmap ((float) voiceIndex, 0.0f, (float) (numVoicesInUse - 1), -1.0f, 1.0f);
}

void ChoirVoice::prepare (const juce::dsp::ProcessSpec& spec, int voiceIndex)
{
    sampleRate = spec.sampleRate;

    // Deterministic per-voice seed so a given voice slot has a consistent
    // "personality" across runs, without every voice sounding identical.
    juce::Random rng (1000 + voiceIndex * 7919);

    panJitter = rng.nextFloat() * 0.16f - 0.08f;
    rateJitterMultiplier = 0.8f + rng.nextFloat() * 0.4f;
    phaseStartRadians = rng.nextFloat() * juce::MathConstants<float>::twoPi;
    delayJitterMs = rng.nextFloat() * 6.0f - 3.0f;
    toneFrequencyHz = 3000.0f + rng.nextFloat() * 6000.0f;
    toneGainDb = rng.nextFloat() * 2.5f - 1.0f;

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    delayLine.setMaximumDelayInSamples ((int) (spec.sampleRate * 0.1)); // 100 ms
    delayLine.prepare (monoSpec);

    toneFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        sampleRate, toneFrequencyHz, 0.7f, juce::Decibels::decibelsToGain (toneGainDb));
    toneFilter.prepare (monoSpec);

    reset();
}

void ChoirVoice::reset()
{
    delayLine.reset();
    toneFilter.reset();
    phase = phaseStartRadians;
}

void ChoirVoice::updateBlockParams (int voiceIndex,
                                     int numVoicesInUse,
                                     float detuneAmount01,
                                     float movementAmount01,
                                     float widthAmount01,
                                     float spreadAmount01)
{
    const float slot = panSlotFor (voiceIndex, numVoicesInUse) + panJitter;
    pan = juce::jlimit (-1.0f, 1.0f, slot * widthAmount01);

    const float baseDelayMs = juce::jmap (spreadAmount01, 0.0f, 1.0f, minSpreadDelayMs, maxSpreadDelayMs)
                               + delayJitterMs;
    baseDelaySamples = (float) (sampleRate * (juce::jmax (1.0f, baseDelayMs) / 1000.0));

    const float depthMs = juce::jmap (detuneAmount01, 0.0f, 1.0f, 0.0f, maxDetuneDepthMs);
    modulationDepthSamples = (float) (sampleRate * (depthMs / 1000.0));

    const float rateHz = juce::jmap (movementAmount01, 0.0f, 1.0f, minMovementHz, maxMovementHz)
                          * rateJitterMultiplier;
    phaseIncrement = juce::MathConstants<float>::twoPi * rateHz / (float) sampleRate;

    gainCompensation = 1.0f / std::sqrt ((float) juce::jmax (1, numVoicesInUse));
}

void ChoirVoice::processSample (float inputSample, float& outL, float& outR)
{
    const float lfo = std::sin (phase);
    float delaySamples = baseDelaySamples + modulationDepthSamples * lfo;
    delaySamples = juce::jlimit (1.0f, (float) delayLine.getMaximumDelayInSamples() - 1.0f, delaySamples);

    delayLine.pushSample (0, inputSample);
    const float delayed = delayLine.popSample (0, delaySamples);
    const float toned = toneFilter.processSample (delayed) * gainCompensation;

    const float leftGain = std::sqrt (0.5f * (1.0f - pan));
    const float rightGain = std::sqrt (0.5f * (1.0f + pan));

    outL += toned * leftGain;
    outR += toned * rightGain;

    phase += phaseIncrement;
    if (phase > juce::MathConstants<float>::twoPi)
        phase -= juce::MathConstants<float>::twoPi;
}
