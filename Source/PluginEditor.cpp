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
    addKnob (distanceKnob, ChoirParam::distance,     "Distance");

    setSize (620, 420);
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

    std::array<KnobControl*, 9> knobs {
        &voicesKnob, &detuneKnob, &movementKnob, &widthKnob, &spreadKnob,
        &distanceKnob, &mixKnob, &reverbKnob, &sizeKnob
    };

    constexpr int numColumns = 5;
    const int columnWidth = area.getWidth() / numColumns;
    const int rowHeight = area.getHeight() / 2;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int row = (int) i / numColumns;
        const int col = (int) i % numColumns;

        auto cell = area.withTrimmedLeft (col * columnWidth)
                        .withTrimmedTop (row * rowHeight)
                        .withWidth (columnWidth)
                        .withHeight (rowHeight);

        auto labelBounds = cell.removeFromBottom (20);
        knobs[i]->slider.setBounds (cell.reduced (4));
        knobs[i]->label.setBounds (labelBounds);
    }
}
