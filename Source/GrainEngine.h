#pragma once

#include <juce_dsp/juce_dsp.h>
#include <random>
#include <vector>

class GrainEngine
{
public:
    GrainEngine();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setGrainSize(float milliseconds);
    void setDensity(float grainsPerSecond);
    void setPitch(float semitones);
    void setSpread(float spreadMs);
    void setFeedback(float feedbackAmount);
    void setWetLevel(float wetAmount);
    void setDelayTime(float milliseconds);

    void processBlock(juce::AudioBuffer<float>& buffer);

private:
    struct Grain
    {
        int channel = 0;
        size_t position = 0;
        size_t length = 0;
        float rate = 1.0f;
        float envelope = 0.0f;
        float envelopeIncrement = 0.0f;
        float fractionalPosition = 0.0f;
        float pan = 0.5f;
    };

    void spawnGrain(int channel);
    float getWindowValue(float env) const;

    std::mt19937 rng { std::random_device{}() };
    std::uniform_real_distribution<float> randomDist { 0.0f, 1.0f };

    std::vector<Grain> grains;
    juce::AudioBuffer<float> delayBuffer;

    double sampleRate = 44100.0;
    size_t writePosition = 0;
    float grainSizeMs = 120.0f;
    float density = 8.0f;
    float pitch = 0.0f;
    float spreadMs = 35.0f;
    float feedback = 0.3f;
    float wet = 0.5f;
    float delayMs = 400.0f;
    float spawnAccumulator = 0.0f;
    float densitySamples = 0.0f;

    static constexpr size_t maxGrains = 128;
};
