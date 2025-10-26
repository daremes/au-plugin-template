#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <vector>

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back(std::make_unique<AudioParameterFloat>("delay", "Delay", NormalisableRange<float>(10.0f, 2000.0f, 0.01f, 0.4f), 350.0f));
    params.push_back(std::make_unique<AudioParameterFloat>("grainSize", "Grain Size", NormalisableRange<float>(10.0f, 500.0f, 0.01f, 0.3f), 120.0f));
    params.push_back(std::make_unique<AudioParameterFloat>("density", "Density", NormalisableRange<float>(0.1f, 40.0f, 0.01f, 0.5f), 8.0f));
    params.push_back(std::make_unique<AudioParameterFloat>("spray", "Spray", NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));
    params.push_back(std::make_unique<AudioParameterFloat>("pitch", "Pitch", NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<AudioParameterFloat>("feedback", "Feedback", NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.35f));
    params.push_back(std::make_unique<AudioParameterFloat>("texture", "Texture", NormalisableRange<float>(0.3f, 3.0f, 0.001f), 1.0f));
    params.push_back(std::make_unique<AudioParameterFloat>("glitch", "Glitch", NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));
    params.push_back(std::make_unique<AudioParameterFloat>("width", "Stereo Width", NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.85f));
    params.push_back(std::make_unique<AudioParameterFloat>("reverbMix", "Reverb Mix", NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));
    params.push_back(std::make_unique<AudioParameterFloat>("reverbSize", "Reverb Size", NormalisableRange<float>(0.1f, 1.0f, 0.001f), 0.7f));
    params.push_back(std::make_unique<AudioParameterFloat>("reverbDamping", "Reverb Damping", NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.45f));
    params.push_back(std::make_unique<AudioParameterFloat>("wetDry", "Wet/Dry", NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.6f));

    return { params.begin(), params.end() };
}
}

CosmicGlitchAudioProcessor::CosmicGlitchAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("CosmicGlitchParameters"), createParameterLayout())
{
    delayTime = parameters.getRawParameterValue("delay");
    grainSize = parameters.getRawParameterValue("grainSize");
    density = parameters.getRawParameterValue("density");
    spray = parameters.getRawParameterValue("spray");
    pitch = parameters.getRawParameterValue("pitch");
    feedback = parameters.getRawParameterValue("feedback");
    texture = parameters.getRawParameterValue("texture");
    glitch = parameters.getRawParameterValue("glitch");
    stereoWidth = parameters.getRawParameterValue("width");
    reverbMix = parameters.getRawParameterValue("reverbMix");
    reverbSize = parameters.getRawParameterValue("reverbSize");
    reverbDamping = parameters.getRawParameterValue("reverbDamping");
    wetDry = parameters.getRawParameterValue("wetDry");
}

void CosmicGlitchAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    grainEngine.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    wetBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), static_cast<juce::uint32>(getTotalNumOutputChannels()) };
    reverb.prepare(spec);
    reverb.reset();

    reverbParameters.roomSize = *reverbSize;
    reverbParameters.damping = *reverbDamping;
    reverbParameters.wetLevel = 1.0f;
    reverbParameters.dryLevel = 0.0f;
    reverbParameters.freezeMode = 0.0f;
    reverbParameters.width = *stereoWidth;
}

void CosmicGlitchAudioProcessor::releaseResources()
{
    wetBuffer.setSize(0, 0);
    reverb.reset();
    grainEngine.reset();
}

bool CosmicGlitchAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void CosmicGlitchAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    wetBuffer.setSize(totalNumOutputChannels, buffer.getNumSamples(), false, false, true);
    wetBuffer.clear();

    grainEngine.setParameters(*delayTime,
                              *grainSize,
                              *density,
                              *spray,
                              *pitch,
                              *feedback,
                              *texture,
                              *glitch,
                              *stereoWidth);

    grainEngine.process(buffer, wetBuffer, *feedback);

    reverbParameters.roomSize = *reverbSize;
    reverbParameters.damping = *reverbDamping;
    reverbParameters.wetLevel = 1.0f;
    reverbParameters.width = *stereoWidth;
    reverbParameters.dryLevel = 0.0f;
    reverb.setParameters(reverbParameters);

    juce::AudioBuffer<float> dryGrainBuffer(wetBuffer);
    juce::dsp::AudioBlock<float> wetBlock { wetBuffer };
    juce::dsp::ProcessContextReplacing<float> context { wetBlock };
    reverb.process(context);

    const float wetDryMix = *wetDry;
    const float reverbLevel = *reverbMix;

    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* dryData = buffer.getWritePointer(channel);
        auto* wetData = wetBuffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const float dryGrain = dryGrainBuffer.getSample(channel, sample);
            const float reverbSample = wetData[sample];
            const float wetSample = juce::jlimit(-1.0f, 1.0f, dryGrain * (1.0f - reverbLevel) + reverbSample * reverbLevel);
            const float drySample = dryData[sample];
            dryData[sample] = drySample * (1.0f - wetDryMix) + wetSample * wetDryMix;
        }
    }

    for (int channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

void CosmicGlitchAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary(*xml, destData);
    }
}

void CosmicGlitchAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorEditor* CosmicGlitchAudioProcessor::createEditor()
{
    return new CosmicGlitchAudioProcessorEditor(*this);
}

//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CosmicGlitchAudioProcessor();
}

