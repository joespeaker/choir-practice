#include "PluginEditor.h"

namespace
{
    const juce::Colour backgroundColour { 0xff1b1e24 };
    const juce::Colour accentColour { 0xffd9a441 };
    const juce::Colour textColour { 0xffe8e6e1 };
}

ChoirPracticeAudioProcessorEditor::ChoirPracticeAudioProcessorEditor (ChoirPracticeAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    titleLabel.setText ("Choir Practice", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (22.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, accentColour);
    addAndMakeVisible (titleLabel);

    addKnob (voicesKnob,   ChoirParam::voices,       "Voices");
    addKnob (detuneKnob,   ChoirParam::detune,       "Detune");
    addKnob (movementKnob, ChoirParam::movement,     "Movement");
    addKnob (widthKnob,    ChoirParam::width,        "Width");
    addKnob (spreadKnob,   ChoirParam::spread,       "Spread");
    addKnob (mixKnob,      ChoirParam::mix,          "Mix");
    addKnob (reverbKnob,   ChoirParam::reverbAmount, "Reverb");
    addKnob (sizeKnob,     ChoirParam::reverbSize,   "Size");

    setSize (620, 320);
}

void ChoirPracticeAudioProcessorEditor::addKnob (KnobControl& knob, const juce::String& paramId, const juce::String& labelText)
{
    knob.slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    knob.slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    knob.slider.setColour (juce::Slider::textBoxTextColourId, textColour);
    knob.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 20);
    addAndMakeVisible (knob.slider);

    knob.label.setText (labelText, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setColour (juce::Label::textColourId, textColour);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramId, knob.slider);
}

void ChoirPracticeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour);
}

void ChoirPracticeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    titleLabel.setBounds (area.removeFromTop (34));
    area.removeFromTop (8);

    std::array<KnobControl*, 8> knobs {
        &voicesKnob, &detuneKnob, &movementKnob, &widthKnob,
        &spreadKnob, &mixKnob, &reverbKnob, &sizeKnob
    };

    const int numKnobs = (int) knobs.size();
    const int knobWidth = area.getWidth() / numKnobs;

    for (int i = 0; i < numKnobs; ++i)
    {
        auto column = area.removeFromLeft (knobWidth);
        auto labelBounds = column.removeFromBottom (20);
        knobs[(size_t) i]->slider.setBounds (column.reduced (4));
        knobs[(size_t) i]->label.setBounds (labelBounds);
    }
}
