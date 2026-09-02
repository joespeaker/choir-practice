#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include <array>

class ChoirPracticeAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ChoirPracticeAudioProcessorEditor (ChoirPracticeAudioProcessor&);
    ~ChoirPracticeAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct KnobControl
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (KnobControl& knob, const juce::String& paramId, const juce::String& labelText);

    ChoirPracticeAudioProcessor& processor;

    KnobControl voicesKnob;
    KnobControl detuneKnob;
    KnobControl movementKnob;
    KnobControl widthKnob;
    KnobControl spreadKnob;
    KnobControl mixKnob;
    KnobControl reverbKnob;
    KnobControl sizeKnob;

    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoirPracticeAudioProcessorEditor)
};
