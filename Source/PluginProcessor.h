#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "GrainEngine.h"

class CosmicGlitchAudioProcessor : public juce::AudioProcessor
{
public:
    CosmicGlitchAudioProcessor();
    ~CosmicGlitchAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "Cosmic Glitch"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }
    GrainEngine& getGrainEngine() { return grainEngine; }

private:
    juce::AudioProcessorValueTreeState parameters;

    GrainEngine grainEngine;
    juce::AudioBuffer<float> wetBuffer;
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParameters;

    juce::AudioParameterFloat* delayTime = nullptr;
    juce::AudioParameterFloat* grainSize = nullptr;
    juce::AudioParameterFloat* density = nullptr;
    juce::AudioParameterFloat* spray = nullptr;
    juce::AudioParameterFloat* pitch = nullptr;
    juce::AudioParameterFloat* feedback = nullptr;
    juce::AudioParameterFloat* texture = nullptr;
    juce::AudioParameterFloat* glitch = nullptr;
    juce::AudioParameterFloat* stereoWidth = nullptr;
    juce::AudioParameterFloat* reverbMix = nullptr;
    juce::AudioParameterFloat* reverbSize = nullptr;
    juce::AudioParameterFloat* reverbDamping = nullptr;
    juce::AudioParameterFloat* wetDry = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CosmicGlitchAudioProcessor)
};

