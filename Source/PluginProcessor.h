#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

#include "ChoirVoice.h"

namespace ChoirParam
{
    static const juce::String voices    = "voices";
    static const juce::String detune    = "detune";
    static const juce::String movement  = "movement";
    static const juce::String width     = "width";
    static const juce::String spread    = "spread";
    static const juce::String mix       = "mix";
    static const juce::String reverbAmount = "reverbAmount";
    static const juce::String reverbSize   = "reverbSize";
    static const juce::String distance     = "distance";
}

class ChoirPracticeAudioProcessor : public juce::AudioProcessor
{
public:
    ChoirPracticeAudioProcessor();
    ~ChoirPracticeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static constexpr int maxVoices = 8;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::array<ChoirVoice, maxVoices> choirVoices;
    juce::dsp::Reverb reverb;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> reverbPredelay { 4096 };

    // Applies a touch of high-frequency air absorption to the choir as
    // "Distance" increases, so farther-away voices read as duller as well
    // as more diffuse.
    juce::dsp::IIR::Filter<float> airFilterL, airFilterR;

    juce::AudioBuffer<float> reverbSendBuffer;
    juce::AudioBuffer<float> dryBuffer;

    std::atomic<float>* voicesParam = nullptr;
    std::atomic<float>* detuneParam = nullptr;
    std::atomic<float>* movementParam = nullptr;
    std::atomic<float>* widthParam = nullptr;
    std::atomic<float>* spreadParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* reverbAmountParam = nullptr;
    std::atomic<float>* reverbSizeParam = nullptr;
    std::atomic<float>* distanceParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoirPracticeAudioProcessor)
};
