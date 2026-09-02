#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    std::unique_ptr<juce::AudioParameterFloat> makePercentParam (const juce::String& id, const juce::String& name, float defaultValue)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id, 1),
            name,
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
            defaultValue,
            juce::AudioParameterFloatAttributes().withLabel ("%"));
    }
}

ChoirPracticeAudioProcessor::ChoirPracticeAudioProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    voicesParam       = apvts.getRawParameterValue (ChoirParam::voices);
    detuneParam       = apvts.getRawParameterValue (ChoirParam::detune);
    movementParam     = apvts.getRawParameterValue (ChoirParam::movement);
    widthParam        = apvts.getRawParameterValue (ChoirParam::width);
    spreadParam       = apvts.getRawParameterValue (ChoirParam::spread);
    mixParam          = apvts.getRawParameterValue (ChoirParam::mix);
    reverbAmountParam = apvts.getRawParameterValue (ChoirParam::reverbAmount);
    reverbSizeParam   = apvts.getRawParameterValue (ChoirParam::reverbSize);
    distanceParam     = apvts.getRawParameterValue (ChoirParam::distance);
}

juce::AudioProcessorValueTreeState::ParameterLayout ChoirPracticeAudioProcessor::createParameterLayout()
{
    return juce::AudioProcessorValueTreeState::ParameterLayout (
        std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID (ChoirParam::voices, 1), "Voices", 1, maxVoices, 5),
        makePercentParam (ChoirParam::detune, "Detune", 35.0f),
        makePercentParam (ChoirParam::movement, "Movement", 40.0f),
        makePercentParam (ChoirParam::width, "Width", 85.0f),
        makePercentParam (ChoirParam::spread, "Spread", 50.0f),
        makePercentParam (ChoirParam::mix, "Mix", 50.0f),
        makePercentParam (ChoirParam::reverbAmount, "Reverb", 25.0f),
        makePercentParam (ChoirParam::reverbSize, "Size", 55.0f),
        makePercentParam (ChoirParam::distance, "Distance", 25.0f));
}

void ChoirPracticeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec voiceSpec;
    voiceSpec.sampleRate = sampleRate;
    voiceSpec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    voiceSpec.numChannels = 1;

    for (int v = 0; v < maxVoices; ++v)
        choirVoices[(size_t) v].prepare (voiceSpec, v);

    juce::dsp::ProcessSpec reverbSpec;
    reverbSpec.sampleRate = sampleRate;
    reverbSpec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    reverbSpec.numChannels = 2;
    reverb.prepare (reverbSpec);
    reverb.reset();

    reverbPredelay.setMaximumDelayInSamples ((int) (sampleRate * 0.05)); // 50 ms
    reverbPredelay.prepare (reverbSpec);
    reverbPredelay.reset();

    juce::dsp::ProcessSpec monoSpec = reverbSpec;
    monoSpec.numChannels = 1;
    airFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 16000.0f);
    airFilterR.coefficients = airFilterL.coefficients;
    airFilterL.prepare (monoSpec);
    airFilterR.prepare (monoSpec);
    airFilterL.reset();
    airFilterR.reset();

    reverbSendBuffer.setSize (2, samplesPerBlock);
    dryBuffer.setSize (2, samplesPerBlock);
}

void ChoirPracticeAudioProcessor::releaseResources()
{
    reverb.reset();
    reverbPredelay.reset();
    airFilterL.reset();
    airFilterR.reset();
    for (auto& v : choirVoices)
        v.reset();
}

bool ChoirPracticeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    const auto inSet = layouts.getMainInputChannelSet();
    if (inSet != juce::AudioChannelSet::mono() && inSet != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void ChoirPracticeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = buffer.getNumChannels();

    const int numVoicesInUse = juce::jlimit (1, maxVoices, (int) std::lround (voicesParam->load()));
    const float detune01   = detuneParam->load() / 100.0f;
    const float movement01 = movementParam->load() / 100.0f;
    const float width01    = widthParam->load() / 100.0f;
    const float spread01   = spreadParam->load() / 100.0f;
    const float mix01      = mixParam->load() / 100.0f;
    const float reverbAmount01 = reverbAmountParam->load() / 100.0f;
    const float reverbSize01   = reverbSizeParam->load() / 100.0f;
    const float distance01     = distanceParam->load() / 100.0f;

    for (int v = 0; v < numVoicesInUse; ++v)
        choirVoices[(size_t) v].updateBlockParams (v, numVoicesInUse, detune01, movement01, width01, spread01);

    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize   = juce::jmap (reverbSize01, 0.0f, 1.0f, 0.2f, 0.95f);
    reverbParams.damping    = 0.5f;
    reverbParams.wetLevel   = 1.0f;
    reverbParams.dryLevel   = 0.0f;
    reverbParams.width      = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters (reverbParams);

    // "Distance": as the choir moves away from the mic, the direct signal
    // gets slightly quieter and duller (air absorption), a bigger share of
    // it goes to the reverb send (more diffuse, less direct — the primary
    // distance cue), and the reverb takes a little longer to respond
    // (predelay), simulating the extra time for reflections to arrive.
    const float airCutoffHz    = juce::jmap (distance01, 0.0f, 1.0f, 16000.0f, 2800.0f);
    const float directGain     = juce::jmap (distance01, 0.0f, 1.0f, 1.0f, 0.72f);
    const float reverbSendScale = juce::jmap (distance01, 0.0f, 1.0f, 1.0f, 1.8f);
    const float predelayMs      = juce::jmap (distance01, 0.0f, 1.0f, 0.0f, 35.0f);

    auto airCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (getSampleRate(), airCutoffHz);
    airFilterL.coefficients = airCoeffs;
    airFilterR.coefficients = airCoeffs;

    const float predelaySamples = (float) (getSampleRate() * (predelayMs / 1000.0));
    reverbPredelay.setDelay (predelaySamples);

    reverbSendBuffer.setSize (2, numSamples, false, false, true);
    reverbSendBuffer.clear();
    dryBuffer.setSize (2, numSamples, false, false, true);

    for (int i = 0; i < numSamples; ++i)
    {
        const float dryL = buffer.getSample (0, i);
        const float dryR = numInputChannels > 1 ? buffer.getSample (1, i) : dryL;
        dryBuffer.setSample (0, i, dryL);
        dryBuffer.setSample (1, i, dryR);

        const float inputSample = numInputChannels > 1 ? 0.5f * (dryL + dryR) : dryL;

        float wetL = 0.0f, wetR = 0.0f;
        for (int v = 0; v < numVoicesInUse; ++v)
            choirVoices[(size_t) v].processSample (inputSample, wetL, wetR);

        wetL = airFilterL.processSample (wetL) * directGain;
        wetR = airFilterR.processSample (wetR) * directGain;

        reverbSendBuffer.setSample (0, i, wetL * reverbAmount01 * reverbSendScale);
        reverbSendBuffer.setSample (1, i, wetR * reverbAmount01 * reverbSendScale);

        buffer.setSample (0, i, wetL);
        if (numOutputChannels > 1)
            buffer.setSample (1, i, wetR);
    }

    if (reverbAmount01 > 0.0001f)
    {
        juce::dsp::AudioBlock<float> predelayBlock (reverbSendBuffer);
        reverbPredelay.process (juce::dsp::ProcessContextReplacing<float> (predelayBlock));

        juce::dsp::AudioBlock<float> reverbBlock (reverbSendBuffer);
        juce::dsp::ProcessContextReplacing<float> reverbContext (reverbBlock);
        reverb.process (reverbContext);

        for (int i = 0; i < numSamples; ++i)
        {
            buffer.setSample (0, i, buffer.getSample (0, i) + reverbSendBuffer.getSample (0, i));
            if (numOutputChannels > 1)
                buffer.setSample (1, i, buffer.getSample (1, i) + reverbSendBuffer.getSample (1, i));
        }
    }

    // Equal-power dry/wet crossfade.
    const float wetGain = std::sin (mix01 * juce::MathConstants<float>::halfPi);
    const float dryGain = std::cos (mix01 * juce::MathConstants<float>::halfPi);

    for (int i = 0; i < numSamples; ++i)
    {
        const float outL = dryBuffer.getSample (0, i) * dryGain + buffer.getSample (0, i) * wetGain;
        buffer.setSample (0, i, outL);

        if (numOutputChannels > 1)
        {
            const float outR = dryBuffer.getSample (1, i) * dryGain + buffer.getSample (1, i) * wetGain;
            buffer.setSample (1, i, outR);
        }
    }
}

juce::AudioProcessorEditor* ChoirPracticeAudioProcessor::createEditor()
{
    return new ChoirPracticeAudioProcessorEditor (*this);
}

void ChoirPracticeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ChoirPracticeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChoirPracticeAudioProcessor();
}
