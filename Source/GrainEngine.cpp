#include "GrainEngine.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float millisecondsToSamples(float ms, double sampleRate)
{
    return static_cast<float>((ms / 1000.0f) * static_cast<float>(sampleRate));
}

float semitoneToRate(float semitone)
{
    return std::pow(2.0f, semitone / 12.0f);
}
}

GrainEngine::GrainEngine()
{
    grains.reserve(maxGrains);
}

void GrainEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    const auto delayBufferSize = static_cast<int>(millisecondsToSamples(2000.0f, sampleRate));
    delayBuffer.setSize((int) spec.numChannels, delayBufferSize);
    delayBuffer.clear();
    writePosition = 0;
    spawnAccumulator = 0.0f;
    densitySamples = sampleRate / juce::jmax(0.01f, density);
}

void GrainEngine::reset()
{
    delayBuffer.clear();
    grains.clear();
    writePosition = 0;
}

void GrainEngine::setGrainSize(float milliseconds)
{
    grainSizeMs = juce::jlimit(10.0f, 1000.0f, milliseconds);
}

void GrainEngine::setDensity(float grainsPerSecond)
{
    density = juce::jlimit(0.5f, 48.0f, grainsPerSecond);
    densitySamples = sampleRate / juce::jmax(0.01f, density);
}

void GrainEngine::setPitch(float semitones)
{
    pitch = juce::jlimit(-24.0f, 24.0f, semitones);
}

void GrainEngine::setSpread(float spread)
{
    spreadMs = juce::jlimit(0.0f, 500.0f, spread);
}

void GrainEngine::setFeedback(float feedbackAmount)
{
    feedback = juce::jlimit(0.0f, 0.98f, feedbackAmount);
}

void GrainEngine::setWetLevel(float wetAmount)
{
    wet = juce::jlimit(0.0f, 1.0f, wetAmount);
}

void GrainEngine::setDelayTime(float milliseconds)
{
    delayMs = juce::jlimit(1.0f, 1500.0f, milliseconds);
}

void GrainEngine::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() == 0)
        return;

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();
    const auto delayBufferSize = delayBuffer.getNumSamples();
    const auto delayOffset = static_cast<size_t>(millisecondsToSamples(delayMs, sampleRate));
    auto** channelWritePointers = buffer.getArrayOfWritePointers();
    auto** delayWritePointers = delayBuffer.getArrayOfWritePointers();
    const auto totalChannels = juce::jmin(numChannels, delayBuffer.getNumChannels());

    for (int sample = 0; sample < numSamples; ++sample)
    {
        spawnAccumulator += 1.0f;
        if (spawnAccumulator >= densitySamples)
        {
            spawnAccumulator -= densitySamples;
            for (int ch = 0; ch < numChannels; ++ch)
                spawnGrain(ch);
        }

        for (int ch = 0; ch < totalChannels; ++ch)
        {
            auto* channelData = channelWritePointers[ch];
            auto* delayData = delayWritePointers[ch];

            const auto drySample = channelData[sample];
            channelData[sample] = 0.0f;
            delayData[writePosition] = drySample + delayData[writePosition] * feedback;
        }

        for (auto& grain : grains)
        {
            if (grain.length == 0)
                continue;

            auto* readData = delayBuffer.getReadPointer(grain.channel);
            auto readPos = (writePosition + delayBufferSize - delayOffset) % delayBufferSize;
            readPos = (readPos + grain.position) % delayBufferSize;

            auto indexFloat = std::fmod(static_cast<float>(readPos) + grain.fractionalPosition,
                                        static_cast<float>(delayBufferSize));
            auto indexInt = static_cast<int>(std::floor(indexFloat));
            auto frac = indexFloat - static_cast<float>(indexInt);
            auto nextIndex = (indexInt + 1) % delayBufferSize;

            auto sampleA = readData[indexInt % delayBufferSize];
            auto sampleB = readData[nextIndex];
            auto window = getWindowValue(grain.envelope);
            auto grainSample = juce::jmap(frac, sampleA, sampleB) * window;

            auto panLeft = std::cos(grain.pan * juce::MathConstants<float>::halfPi);
            auto panRight = std::sin(grain.pan * juce::MathConstants<float>::halfPi);

            if (numChannels > 0)
                channelWritePointers[0][sample] += grainSample * panLeft * wet;
            if (numChannels > 1)
                channelWritePointers[1][sample] += grainSample * panRight * wet;

            grain.fractionalPosition += grain.rate;
            grain.envelope += grain.envelopeIncrement;
            grain.position += 1;

            if (grain.position >= grain.length)
                grain.length = 0;
        }

        writePosition = (writePosition + 1) % static_cast<size_t>(delayBufferSize);
    }

    grains.erase(std::remove_if(grains.begin(), grains.end(), [](const Grain& g) { return g.length == 0; }), grains.end());
}

void GrainEngine::spawnGrain(int channel)
{
    if (grains.size() >= maxGrains)
        return;

    Grain grain;
    grain.channel = channel;
    grain.position = 0;
    auto lengthMs = juce::jmax(10.0f, grainSizeMs + (randomDist(rng) - 0.5f) * spreadMs);
    grain.length = static_cast<size_t>(millisecondsToSamples(lengthMs, sampleRate));
    grain.length = juce::jmax<size_t>(32, grain.length);
    grain.rate = semitoneToRate(pitch + (randomDist(rng) - 0.5f) * 3.0f);
    grain.envelope = 0.0f;
    grain.envelopeIncrement = 1.0f / static_cast<float>(grain.length);
    grain.fractionalPosition = 0.0f;
    grain.pan = juce::jlimit(0.0f, 1.0f, randomDist(rng));

    grains.push_back(grain);
}

float GrainEngine::getWindowValue(float env) const
{
    auto t = juce::jlimit(0.0f, 1.0f, env);
    return std::sin(t * juce::MathConstants<float>::pi);
}
