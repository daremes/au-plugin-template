#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

#include "GrainEngine.h"

class CosmicGrainDelayAudioProcessor : public juce::AudioProcessor
{
public:
    CosmicGrainDelayAudioProcessor();
    ~CosmicGrainDelayAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override { juce::ignoreUnused(index); }
    const juce::String getProgramName(int index) override { juce::ignoreUnused(index); return {}; }
    void changeProgramName(int index, const juce::String& newName) override { juce::ignoreUnused(index, newName); }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }
    GrainEngine::VisualSnapshot getGrainVisualSnapshot() const { return grainEngine.getVisualSnapshot(); }

    static constexpr std::array<const char*, 19> delayDivisionLabels {
        "Free",
        "1/1",
        "1/1.",
        "1/1T",
        "1/2",
        "1/2.",
        "1/2T",
        "1/4",
        "1/4.",
        "1/4T",
        "1/8",
        "1/8.",
        "1/8T",
        "1/16",
        "1/16.",
        "1/16T",
        "1/32",
        "1/32.",
        "1/32T"
    };

    static constexpr std::array<float, delayDivisionLabels.size()> delayDivisionBeats {
        0.0f,      // Free
        4.0f,      // 1/1
        6.0f,      // 1/1.
        4.0f * (2.0f / 3.0f), // 1/1T -> 8/3 beats
        2.0f,      // 1/2
        3.0f,      // 1/2.
        2.0f * (2.0f / 3.0f), // 1/2T
        1.0f,      // 1/4
        1.5f,      // 1/4.
        1.0f * (2.0f / 3.0f), // 1/4T
        0.5f,      // 1/8
        0.75f,     // 1/8.
        0.5f * (2.0f / 3.0f), // 1/8T
        0.25f,     // 1/16
        0.375f,    // 1/16.
        0.25f * (2.0f / 3.0f), // 1/16T
        0.125f,    // 1/32
        0.1875f,   // 1/32.
        0.125f * (2.0f / 3.0f) // 1/32T
    };

    static_assert(delayDivisionLabels.size() > 0, "Delay division labels must not be empty");
    static_assert(delayDivisionLabels.size() == delayDivisionBeats.size(),
        "Delay division tables must remain aligned");

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float resolveDelayMilliseconds(float freeDelayMs, bool syncEnabled, float divisionIndex, double bpm) const;
    void applyDistortion(juce::AudioBuffer<float>& buffer, float drive, float tone, float mix, bool enabled);

    GrainEngine grainEngine;
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;
    juce::AudioBuffer<float> distortionBuffer;
    juce::dsp::WaveShaper<float> distortionShaper;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> distortionToneFilter;
    double currentSampleRate = 44100.0;
    juce::AudioProcessorValueTreeState parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CosmicGrainDelayAudioProcessor)
};
