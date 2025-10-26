#include "PluginProcessor.h"
#include "PluginEditor.h"

CosmicGrainDelayAudioProcessor::CosmicGrainDelayAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

void CosmicGrainDelayAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), static_cast<juce::uint32>(getTotalNumOutputChannels()) };
    grainEngine.prepare(spec);
    grainEngine.reset();
    reverb.reset();
}

void CosmicGrainDelayAudioProcessor::releaseResources()
{
}

void CosmicGrainDelayAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto* grainSize = parameters.getRawParameterValue("grainSize");
    auto* density = parameters.getRawParameterValue("density");
    auto* pitch = parameters.getRawParameterValue("pitch");
    auto* spread = parameters.getRawParameterValue("spread");
    auto* feedback = parameters.getRawParameterValue("feedback");
    auto* wet = parameters.getRawParameterValue("grainWet");
    auto* delay = parameters.getRawParameterValue("delayTime");
    auto* reverbMix = parameters.getRawParameterValue("reverbMix");
    auto* reverbSize = parameters.getRawParameterValue("reverbSize");
    auto* reverbDamping = parameters.getRawParameterValue("reverbDamping");
    auto* reverbWidth = parameters.getRawParameterValue("reverbWidth");
    auto* reverbFreeze = parameters.getRawParameterValue("reverbFreeze");

    grainEngine.setGrainSize(*grainSize);
    grainEngine.setDensity(*density);
    grainEngine.setPitch(*pitch);
    grainEngine.setSpread(*spread);
    grainEngine.setFeedback(*feedback);
    grainEngine.setWetLevel(*wet);
    grainEngine.setDelayTime(*delay);

    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    grainEngine.processBlock(buffer);

    reverbParams.roomSize = *reverbSize;
    reverbParams.damping = *reverbDamping;
    reverbParams.wetLevel = 1.0f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = *reverbWidth;
    reverbParams.freezeMode = (*reverbFreeze >= 0.5f) ? 1.0f : 0.0f;
    reverb.setParameters(reverbParams);

    juce::AudioBuffer<float> reverbBuffer;
    reverbBuffer.makeCopyOf(buffer);

    juce::dsp::AudioBlock<float> reverbBlock(reverbBuffer);
    juce::dsp::ProcessContextReplacing<float> reverbContext(reverbBlock);
    reverb.process(reverbContext);

    const auto mix = *reverbMix;
    const auto grainWet = *wet;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* dry = dryBuffer.getReadPointer(juce::jmin(channel, dryBuffer.getNumChannels() - 1));
        auto* wetGrain = buffer.getWritePointer(channel);
        auto* wetReverb = reverbBuffer.getReadPointer(juce::jmin(channel, reverbBuffer.getNumChannels() - 1));

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            auto combinedWet = wetGrain[sample] * (1.0f - mix) + wetReverb[sample] * mix;
            wetGrain[sample] = dry[sample] * (1.0f - grainWet) + combinedWet * grainWet;
        }
    }
}

void CosmicGrainDelayAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState())
        if (auto xml = state.createXml())
            copyXmlToBinary(*xml, destData);
}

void CosmicGrainDelayAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout CosmicGrainDelayAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("grainSize", "Grain Size", juce::NormalisableRange<float>(20.0f, 500.0f, 0.01f), 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("density", "Density", juce::NormalisableRange<float>(0.5f, 32.0f, 0.01f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pitch", "Pitch", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("spread", "Spread", juce::NormalisableRange<float>(0.0f, 250.0f, 0.01f), 35.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayTime", "Delay", juce::NormalisableRange<float>(10.0f, 1500.0f, 0.01f), 400.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("feedback", "Feedback", juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("grainWet", "Grain Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbMix", "Reverb Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbSize", "Reverb Size", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbDamping", "Reverb Damping", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbWidth", "Reverb Width", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.9f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("reverbFreeze", "Reverb Freeze", false));

    return { params.begin(), params.end() };
}

juce::AudioProcessorEditor* CosmicGrainDelayAudioProcessor::createEditor()
{
    return new CosmicGrainDelayAudioProcessorEditor(*this, parameters);
}
