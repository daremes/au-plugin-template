#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

CosmicGrainDelayAudioProcessor::CosmicGrainDelayAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

void CosmicGrainDelayAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), static_cast<juce::uint32>(getTotalNumOutputChannels()) };
    grainEngine.prepare(spec);
    grainEngine.reset();
    reverb.reset();

    distortionShaper.reset();
    distortionShaper.prepare(spec);
    distortionShaper.functionToUse = [](float x) { return std::tanh(x); };

    distortionToneFilter.reset();
    distortionToneFilter.state = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 2000.0f);
    distortionToneFilter.prepare(spec);

    distortionBuffer.setSize(static_cast<int>(getTotalNumOutputChannels()), samplesPerBlock);
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
    auto* grainScatter = parameters.getRawParameterValue("grainScatter");
    auto* grainEnvelopeShape = parameters.getRawParameterValue("grainEnvelopeShape");
    auto* grainPitchJitter = parameters.getRawParameterValue("grainPitchJitter");
    auto* feedback = parameters.getRawParameterValue("feedback");
    auto* wet = parameters.getRawParameterValue("grainWet");
    auto* delay = parameters.getRawParameterValue("delayTime");
    auto* delaySync = parameters.getRawParameterValue("delaySync");
    auto* delayDivision = parameters.getRawParameterValue("delayDivision");
    auto* distortionEnabled = parameters.getRawParameterValue("distortionEnabled");
    auto* distortionDrive = parameters.getRawParameterValue("distortionDrive");
    auto* distortionTone = parameters.getRawParameterValue("distortionTone");
    auto* distortionMix = parameters.getRawParameterValue("distortionMix");
    auto* reverbMix = parameters.getRawParameterValue("reverbMix");
    auto* reverbSize = parameters.getRawParameterValue("reverbSize");
    auto* reverbDamping = parameters.getRawParameterValue("reverbDamping");
    auto* reverbWidth = parameters.getRawParameterValue("reverbWidth");
    auto* reverbFreeze = parameters.getRawParameterValue("reverbFreeze");

    grainEngine.setGrainSize(*grainSize);
    grainEngine.setDensity(*density);
    grainEngine.setPitch(*pitch);
    grainEngine.setSpread(*spread);
    grainEngine.setScatter(*grainScatter);
    grainEngine.setEnvelopeShape(*grainEnvelopeShape);
    grainEngine.setPitchJitter(*grainPitchJitter);
    grainEngine.setFeedback(*feedback);

    double bpm = 0.0;
    if (auto* head = getPlayHead())
        if (auto position = head->getPosition())
            if (auto bpmValue = position->getBpm())
                bpm = *bpmValue;

    const auto resolvedDelay = resolveDelayMilliseconds(*delay, *delaySync >= 0.5f, *delayDivision, bpm);
    grainEngine.setDelayTime(resolvedDelay);

    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    grainEngine.processBlock(buffer);

    applyDistortion(buffer, *distortionDrive, *distortionTone, *distortionMix, *distortionEnabled >= 0.5f);

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

    const auto mix = reverbMix->load();
    const auto grainWet = wet->load();
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
    if (auto state = parameters.copyState(); state.isValid())
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

    params.push_back(std::make_unique<juce::AudioParameterFloat>("grainSize", "Nebula Size", juce::NormalisableRange<float>(20.0f, 500.0f, 0.01f), 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("density", "Meteor Swarm", juce::NormalisableRange<float>(0.5f, 512.0f, 0.01f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pitch", "Orbit Shift", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("spread", "Comet Spread", juce::NormalisableRange<float>(0.0f, 250.0f, 0.01f), 35.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("grainScatter", "Wormhole Scatter", juce::NormalisableRange<float>(0.0f, 200.0f, 0.01f), 25.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("grainEnvelopeShape", "Gravity Envelope", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("grainPitchJitter", "Quantum Drift", juce::NormalisableRange<float>(0.0f, 12.0f, 0.001f), 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayTime", "Warp Drift", juce::NormalisableRange<float>(10.0f, 1500.0f, 0.01f), 400.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("delaySync", "Temporal Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayDivision", "Warp Division",
        juce::NormalisableRange<float>(0.0f, static_cast<float>(CosmicGrainDelayAudioProcessor::delayDivisionLabels.size() - 1), 1.0f),
        5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("feedback", "Orbit Feedback", juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("distortionEnabled", "Meteor Ignite", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("distortionDrive", "Meteor Burn", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("distortionTone", "Burn Tone", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("distortionMix", "Burn Blend", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("grainWet", "Stardust Blend", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbMix", "Nebula Wash", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbSize", "Nebula Horizon", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbDamping", "Stellar Damping", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbWidth", "Cosmic Width", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.9f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("reverbFreeze", "Space Freeze", false));

    return { params.begin(), params.end() };
}

float CosmicGrainDelayAudioProcessor::resolveDelayMilliseconds(float freeDelayMs, bool syncEnabled, float divisionIndex, double bpm) const
{
    auto clampedFree = juce::jlimit(10.0f, 1500.0f, freeDelayMs);
    if (!syncEnabled)
        return clampedFree;

    const auto maxIndex = static_cast<int>(delayDivisionBeats.size() - 1);
    auto index = juce::jlimit(0, maxIndex, static_cast<int>(std::round(divisionIndex)));
    if (index == 0 || bpm <= 0.0)
        return clampedFree;

    const auto beats = delayDivisionBeats[static_cast<size_t>(index)];
    const auto ms = static_cast<float>((60000.0 / (bpm <= 0.0 ? 120.0 : bpm)) * beats);
    return juce::jlimit(10.0f, 1500.0f, ms);
}

void CosmicGrainDelayAudioProcessor::applyDistortion(juce::AudioBuffer<float>& buffer, float drive, float tone, float mix, bool enabled)
{
    if ((!enabled && mix <= 0.0f) || buffer.getNumSamples() == 0)
        return;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (distortionBuffer.getNumChannels() < numChannels || distortionBuffer.getNumSamples() < numSamples)
        distortionBuffer.setSize(numChannels, numSamples, false, false, true);

    distortionBuffer.makeCopyOf(buffer);

    juce::dsp::AudioBlock<float> block(distortionBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    const auto driveAmount = juce::jmap(drive, 0.0f, 1.0f, 1.0f, 10.0f);
    distortionBuffer.applyGain(driveAmount);
    distortionShaper.process(context);

    const auto cutoff = juce::jmap(tone, 0.0f, 1.0f, 800.0f, 8000.0f);
    distortionToneFilter.state = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, cutoff);
    distortionToneFilter.process(context);

    auto blend = juce::jlimit(0.0f, 1.0f, enabled ? mix : 0.0f);
    if (blend <= 0.0f)
        return;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* dry = buffer.getWritePointer(channel);
        auto* wet = distortionBuffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
            dry[sample] = dry[sample] * (1.0f - blend) + wet[sample] * blend;
    }
}

juce::AudioProcessorEditor* CosmicGrainDelayAudioProcessor::createEditor()
{
    return new CosmicGrainDelayAudioProcessorEditor(*this, parameters);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CosmicGrainDelayAudioProcessor();
}
