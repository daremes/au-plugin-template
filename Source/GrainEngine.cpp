#include "GrainEngine.h"

#include <cmath>

namespace
{
constexpr float maxDelaySeconds = 6.0f;
constexpr float minDensity = 0.05f;

float hannWindow(float phase)
{
    return 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * juce::jlimit(0.0f, 1.0f, phase));
}
}

void GrainEngine::prepare(double newSampleRate, int /*maximumBlockSize*/, int numChannels)
{
    sampleRate = juce::jmax(1000.0, newSampleRate);
    const auto requiredSize = static_cast<int>(std::ceil(sampleRate * maxDelaySeconds)) + 1;

    delayBuffer.setSize(juce::jmax(1, numChannels), requiredSize);
    delayBuffer.clear();

    delayBufferSize = requiredSize;
    writePosition = 0;

    grains.clear();

    samplesUntilNextGrain = 0.0f;
    grainInterval = sampleRate / juce::jmax(grainDensity, minDensity);
}

void GrainEngine::reset()
{
    delayBuffer.clear();
    grains.clear();
    writePosition = 0;
    samplesUntilNextGrain = 0.0f;
}

void GrainEngine::setParameters(float delayMilliseconds,
                                float grainSizeMilliseconds,
                                float densityPerSecond,
                                float spray,
                                float pitchSemitones,
                                float /*feedbackAmount*/,
                                float texture,
                                float glitch,
                                float stereoWidth)
{
    delayTimeSamples = juce::jlimit(1.0f, static_cast<float>(delayBufferSize - 2), delayMilliseconds * static_cast<float>(sampleRate) / 1000.0f);
    grainSizeSamples = juce::jlimit(1.0f, static_cast<float>(delayBufferSize - 2), grainSizeMilliseconds * static_cast<float>(sampleRate) / 1000.0f);

    grainDensity = juce::jlimit(minDensity, 60.0f, densityPerSecond);
    grainSpray = juce::jlimit(0.0f, 1.0f, spray);
    pitchRatio = std::pow(2.0f, pitchSemitones / 12.0f);
    grainTexture = juce::jlimit(0.2f, 3.5f, texture);
    glitchChance = juce::jlimit(0.0f, 1.0f, glitch);
    widthAmount = juce::jlimit(0.0f, 1.0f, stereoWidth);

    grainInterval = static_cast<float>(sampleRate / juce::jmax(grainDensity, minDensity));
}

void GrainEngine::process(juce::AudioBuffer<float>& inputBuffer,
                          juce::AudioBuffer<float>& wetBuffer,
                          float feedbackAmount)
{
    jassert(inputBuffer.getNumChannels() == wetBuffer.getNumChannels());
    const auto numSamples = inputBuffer.getNumSamples();

    int localWritePosition = writePosition;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        samplesUntilNextGrain -= 1.0f;
        while (samplesUntilNextGrain <= 0.0f)
        {
            spawnGrain(localWritePosition);
            samplesUntilNextGrain += grainInterval;
        }

        float grainSampleL = 0.0f;
        float grainSampleR = 0.0f;

        for (auto grain = grains.begin(); grain != grains.end();)
        {
            auto& g = *grain;

            const auto env = std::pow(hannWindow(g.envelopePhase), g.shape);

            float sampleSum = 0.0f;
            const int channelsToProcess = juce::jmax(1, delayBuffer.getNumChannels());
            for (int channel = 0; channel < channelsToProcess; ++channel)
                sampleSum += readFromDelayBuffer(channel, g.position);

            sampleSum /= static_cast<float>(channelsToProcess);

            const float equalPowerPan = juce::jlimit(0.0f, 1.0f, 0.5f + (g.pan - 0.5f) * widthAmount);
            const float leftGain = std::cos(juce::MathConstants<float>::halfPi * equalPowerPan);
            const float rightGain = std::sin(juce::MathConstants<float>::halfPi * equalPowerPan);

            grainSampleL += sampleSum * leftGain * env;
            grainSampleR += sampleSum * rightGain * env;

            g.position += g.increment;

            while (g.position < 0.0f)
                g.position += static_cast<float>(delayBufferSize);
            while (g.position >= static_cast<float>(delayBufferSize))
                g.position -= static_cast<float>(delayBufferSize);

            g.envelopePhase += g.envelopeDelta;
            g.samplesRemaining -= 1;

            if (g.samplesRemaining <= 0)
                grain = grains.erase(grain);
            else
                ++grain;
        }

        for (int channel = 0; channel < wetBuffer.getNumChannels(); ++channel)
        {
            const bool isEven = (channel % 2) == 0;
            const float sampleValue = isEven ? grainSampleL : grainSampleR;
            wetBuffer.setSample(channel, sample, wetBuffer.getSample(channel, sample) + sampleValue);
        }

        for (int channel = 0; channel < inputBuffer.getNumChannels(); ++channel)
        {
            const auto drySample = inputBuffer.getSample(channel, sample);
            const float feedbackSample = (channel == 0 ? grainSampleL : grainSampleR) * feedbackAmount;

            float writeSample = drySample + feedbackSample;
            delayBuffer.setSample(channel, localWritePosition, writeSample);
        }

        for (int channel = inputBuffer.getNumChannels(); channel < delayBuffer.getNumChannels(); ++channel)
            delayBuffer.setSample(channel, localWritePosition, 0.0f);

        if (++localWritePosition >= delayBufferSize)
            localWritePosition = 0;
    }

    writePosition = localWritePosition;
}

float GrainEngine::readFromDelayBuffer(int channel, float position) const
{
    if (delayBufferSize == 0)
        return 0.0f;

    const auto bufferSize = static_cast<float>(delayBufferSize);
    position = std::fmod(position, bufferSize);
    if (position < 0.0f)
        position += bufferSize;

    const int index0 = static_cast<int>(position);
    const int index1 = (index0 + 1) % delayBufferSize;
    const float frac = position - static_cast<float>(index0);

    const float sample0 = delayBuffer.getSample(channel % delayBuffer.getNumChannels(), index0);
    const float sample1 = delayBuffer.getSample(channel % delayBuffer.getNumChannels(), index1);

    return sample0 + (sample1 - sample0) * frac;
}

void GrainEngine::spawnGrain(int currentWritePosition)
{
    if (delayBufferSize <= 0)
        return;

    Grain newGrain;

    const auto lengthSamples = static_cast<int>(juce::jlimit(1.0f, static_cast<float>(delayBufferSize - 2), grainSizeSamples));
    newGrain.samplesRemaining = lengthSamples;
    newGrain.envelopePhase = 0.0f;
    newGrain.envelopeDelta = 1.0f / static_cast<float>(juce::jmax(1, lengthSamples));
    newGrain.shape = grainTexture;

    const float sprayRange = grainSpray * static_cast<float>(lengthSamples);
    const float sprayOffset = (random.nextFloat() * 2.0f - 1.0f) * sprayRange;
    float startOffset = delayTimeSamples + sprayOffset;
    startOffset = juce::jlimit(1.0f, static_cast<float>(delayBufferSize - 2), startOffset);

    float startPosition = static_cast<float>(currentWritePosition) - startOffset;
    while (startPosition < 0.0f)
        startPosition += static_cast<float>(delayBufferSize);
    while (startPosition >= static_cast<float>(delayBufferSize))
        startPosition -= static_cast<float>(delayBufferSize);

    newGrain.position = startPosition;
    newGrain.increment = pitchRatio;

    if (glitchChance > 0.0f && random.nextFloat() < glitchChance)
    {
        const float glitchMode = random.nextFloat();
        if (glitchMode < 0.33f)
            newGrain.increment = -pitchRatio; // reverse
        else if (glitchMode < 0.66f)
            newGrain.increment = pitchRatio * (1.5f + random.nextFloat());
        else
            newGrain.envelopeDelta *= 0.5f; // stretched grain
    }

    newGrain.pan = random.nextFloat();

    grains.push_back(newGrain);
}

