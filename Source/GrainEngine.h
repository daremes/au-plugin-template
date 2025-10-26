#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <vector>

class GrainEngine
{
public:
    void prepare(double newSampleRate, int maximumBlockSize, int numChannels);
    void reset();

    void setParameters(float delayMilliseconds,
                       float grainSizeMilliseconds,
                       float densityPerSecond,
                       float spray,
                       float pitchSemitones,
                       float feedbackAmount,
                       float texture,
                       float glitch,
                       float stereoWidth);

    void process(juce::AudioBuffer<float>& inputBuffer,
                 juce::AudioBuffer<float>& wetBuffer,
                 float feedbackAmount);

    double getSampleRate() const noexcept { return sampleRate; }

private:
    struct Grain
    {
        float position = 0.0f;
        float increment = 1.0f;
        int samplesRemaining = 0;
        float envelopePhase = 0.0f;
        float envelopeDelta = 0.0f;
        float pan = 0.5f;
        float shape = 1.0f;
    };

    float readFromDelayBuffer(int channel, float position) const;
    void spawnGrain(int writePosition);

    juce::AudioBuffer<float> delayBuffer;
    int delayBufferSize = 0;
    int writePosition = 0;

    double sampleRate = 44100.0;
    juce::Random random;

    std::vector<Grain> grains;

    // Parameter cache
    float delayTimeSamples = 4410.0f;
    float grainSizeSamples = 441.0f;
    float grainDensity = 4.0f; // grains per second
    float grainSpray = 0.0f;
    float pitchRatio = 1.0f;
    float grainTexture = 1.0f;
    float glitchChance = 0.0f;
    float widthAmount = 0.5f;

    float samplesUntilNextGrain = 0.0f;
    float grainInterval = 4410.0f;
};

