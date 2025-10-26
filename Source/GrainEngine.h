#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <random>

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
    void setScatter(float milliseconds);
    void setEnvelopeShape(float shape);
    void setPitchJitter(float semitones);

    void processBlock(juce::AudioBuffer<float>& buffer);

    // Telemetry structures mirrored to the editor so it can render a live particle view
    // without touching the real-time grain pool directly. The audio thread populates a
    // double-buffered snapshot once per block, and the GUI polls using getVisualSnapshot().
    struct VisualGrain
    {
        float pan = 0.5f;           // 0 = hard left, 1 = hard right
        float age = 0.0f;           // grain progress 0-1
        float durationSeconds = 0.0f;
        float pitchSemitone = 0.0f; // signed relative pitch
        float envelope = 0.0f;      // current window value 0-1
    };

    struct VisualSnapshot
    {
        std::array<VisualGrain, 256> grains {};
        size_t grainCount = 0;
        size_t activeGrains = 0;
        float spawnRatePerSecond = 0.0f;
        float delayTimeMs = 0.0f;
    };

    VisualSnapshot getVisualSnapshot() const;

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
        int startOffset = 0;
        bool active = false;
    };

    static constexpr size_t maxGrains = 1024;

    void resetPool();
    Grain* allocateGrain(size_t& indexOut);
    void releaseGrainAtActiveIndex(size_t activeListIndex);
    void updateSpawnInterval(int numChannels);
    void updateVisualSnapshot();
    void spawnGrain(int channel);
    float getWindowValue(float env) const;

    std::mt19937 rng { std::random_device{}() };
    std::uniform_real_distribution<float> randomDist { 0.0f, 1.0f };

    std::array<Grain, maxGrains> grainPool {};
    std::array<uint16_t, maxGrains> activeIndices {};
    std::array<uint16_t, maxGrains> freeIndices {};
    size_t activeGrainCount = 0;
    size_t freeGrainCount = maxGrains;
    juce::AudioBuffer<float> delayBuffer;

    double sampleRate = 44100.0;
    size_t writePosition = 0;
    float grainSizeMs = 120.0f;
    float density = 8.0f;
    float pitch = 0.0f;
    float spreadMs = 35.0f;
    float feedback = 0.3f;
    float delayMs = 400.0f;
    float spawnAccumulator = 0.0f;
    float scatterMs = 20.0f;
    size_t scatterSamples = 0;
    float envelopeShape = 0.5f;
    float pitchJitter = 0.0f;
    float spawnIntervalSamples = 1.0f;
    juce::LinearSmoothedValue<float> smoothedDelaySamples;
    VisualSnapshot visualSnapshots[2] {};
    std::atomic<int> visualSnapshotIndex { 0 };
};
