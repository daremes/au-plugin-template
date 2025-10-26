#include "GrainEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>

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
    resetPool();
}

void GrainEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    const auto delayBufferSize = static_cast<int>(millisecondsToSamples(2000.0f, sampleRate));
    delayBuffer.setSize((int) spec.numChannels, delayBufferSize);
    delayBuffer.clear();
    writePosition = 0;
    spawnAccumulator = 0.0f;
    smoothedDelaySamples.reset(sampleRate, 0.02);
    smoothedDelaySamples.setCurrentAndTargetValue(millisecondsToSamples(delayMs, sampleRate));
    resetPool();
}

void GrainEngine::reset()
{
    delayBuffer.clear();
    writePosition = 0;
    spawnAccumulator = 0.0f;
    smoothedDelaySamples.setCurrentAndTargetValue(millisecondsToSamples(delayMs, sampleRate));
    resetPool();
}

void GrainEngine::setGrainSize(float milliseconds)
{
    grainSizeMs = juce::jlimit(10.0f, 1000.0f, milliseconds);
}

void GrainEngine::setDensity(float grainsPerSecond)
{
    density = juce::jlimit(0.5f, 512.0f, grainsPerSecond);
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
    // Wet/dry blending now happens in the processor so we keep this placeholder
    // to minimise API churn while allowing future per-grain gain scaling.
    juce::ignoreUnused(wetAmount);
}

void GrainEngine::setDelayTime(float milliseconds)
{
    delayMs = juce::jlimit(1.0f, 1500.0f, milliseconds);
    smoothedDelaySamples.setTargetValue(millisecondsToSamples(delayMs, sampleRate));
}

void GrainEngine::setScatter(float milliseconds)
{
    scatterMs = juce::jlimit(0.0f, 500.0f, milliseconds);
    scatterSamples = static_cast<size_t>(juce::roundToInt(std::min(millisecondsToSamples(scatterMs, sampleRate),
        static_cast<float>(delayBuffer.getNumSamples()))));
}

void GrainEngine::setEnvelopeShape(float shape)
{
    envelopeShape = juce::jlimit(0.0f, 1.0f, shape);
}

void GrainEngine::setPitchJitter(float semitones)
{
    pitchJitter = juce::jlimit(0.0f, 12.0f, semitones);
}

void GrainEngine::resetPool()
{
    // Pool reset keeps allocation predictable and avoids per-sample heap churn
    // when we scale up to hundreds of overlapping grains.
    activeGrainCount = 0;
    freeGrainCount = maxGrains;

    for (size_t i = 0; i < maxGrains; ++i)
    {
        grainPool[i] = Grain{};
        grainPool[i].active = false;
        activeIndices[i] = 0;
        freeIndices[i] = static_cast<uint16_t>(maxGrains - 1 - i);
    }

    visualSnapshots[0] = VisualSnapshot{};
    visualSnapshots[1] = VisualSnapshot{};
    visualSnapshotIndex.store(0, std::memory_order_relaxed);
}

GrainEngine::Grain* GrainEngine::allocateGrain(size_t& indexOut)
{
    if (freeGrainCount == 0)
        return nullptr;

    auto slot = freeIndices[--freeGrainCount];
    indexOut = static_cast<size_t>(slot);
    auto& grain = grainPool[indexOut];
    grain = Grain{};
    grain.active = true;
    activeIndices[activeGrainCount++] = static_cast<uint16_t>(indexOut);
    return &grain;
}

void GrainEngine::releaseGrainAtActiveIndex(size_t activeListIndex)
{
    if (activeListIndex >= activeGrainCount)
        return;

    const auto poolIndex = activeIndices[activeListIndex];
    grainPool[poolIndex].active = false;

    if (activeListIndex != activeGrainCount - 1)
        activeIndices[activeListIndex] = activeIndices[activeGrainCount - 1];

    --activeGrainCount;
    freeIndices[freeGrainCount++] = poolIndex;
}

void GrainEngine::updateSpawnInterval(int numChannels)
{
    // Treat the density control as a global grains-per-second value and derive
    // per-channel spawn intervals so stereo instances stay predictable.
    const auto channelCount = juce::jmax(1, numChannels);
    const auto effectiveDensity = juce::jmax(0.5f, density);
    const auto eventsPerSecond = effectiveDensity / static_cast<float>(channelCount);

    if (eventsPerSecond <= 0.0f)
    {
        spawnIntervalSamples = std::numeric_limits<float>::max();
        return;
    }

    spawnIntervalSamples = static_cast<float>(sampleRate) / eventsPerSecond;
    spawnIntervalSamples = juce::jmax(1.0f, spawnIntervalSamples);
}

void GrainEngine::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() == 0)
        return;

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();
    const auto delayBufferSize = delayBuffer.getNumSamples();
    auto channelWritePointers = buffer.getArrayOfWritePointers();
    auto delayWritePointers = delayBuffer.getArrayOfWritePointers();
    const auto totalChannels = juce::jmin(numChannels, delayBuffer.getNumChannels());
    updateSpawnInterval(totalChannels);

    smoothedDelaySamples.setTargetValue(millisecondsToSamples(delayMs, sampleRate));
    const bool spawnEnabled = std::isfinite(spawnIntervalSamples) &&
        spawnIntervalSamples < std::numeric_limits<float>::max();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto delayOffset = static_cast<int>(juce::roundToInt(smoothedDelaySamples.getNextValue()));

        if (spawnEnabled)
        {
            spawnAccumulator += 1.0f;
            while (spawnAccumulator >= spawnIntervalSamples)
            {
                spawnAccumulator -= spawnIntervalSamples;
                for (int ch = 0; ch < totalChannels; ++ch)
                    spawnGrain(ch);
            }
        }
        else
        {
            spawnAccumulator = 0.0f;
        }

        for (int ch = 0; ch < totalChannels; ++ch)
        {
            auto* channelData = channelWritePointers[ch];
            auto* delayData = delayWritePointers[ch];

            const auto drySample = channelData[sample];
            channelData[sample] = 0.0f;
            delayData[writePosition] = drySample + delayData[writePosition] * feedback;
        }

        size_t activeIndex = 0;
        while (activeIndex < activeGrainCount)
        {
            auto poolIndex = activeIndices[activeIndex];
            auto& grain = grainPool[poolIndex];

            if (!grain.active || grain.length == 0)
            {
                releaseGrainAtActiveIndex(activeIndex);
                continue;
            }

            auto* readData = delayBuffer.getReadPointer(grain.channel);
            auto basePos = (static_cast<int>(writePosition) + delayBufferSize - delayOffset) % delayBufferSize;
            auto readPos = (basePos - grain.startOffset + delayBufferSize) % delayBufferSize;
            readPos = (readPos + static_cast<int>(grain.position)) % delayBufferSize;

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
                channelWritePointers[0][sample] += grainSample * panLeft;
            if (numChannels > 1)
                channelWritePointers[1][sample] += grainSample * panRight;

            grain.fractionalPosition += grain.rate;
            grain.envelope += grain.envelopeIncrement;
            grain.position += 1;

            if (grain.position >= grain.length)
            {
                releaseGrainAtActiveIndex(activeIndex);
                continue;
            }

            ++activeIndex;
        }

        writePosition = (writePosition + 1) % static_cast<size_t>(delayBufferSize);
    }

    updateVisualSnapshot();
}

void GrainEngine::spawnGrain(int channel)
{
    if (channel < 0 || channel >= delayBuffer.getNumChannels())
        return;

    size_t poolIndex = 0;
    if (auto* grain = allocateGrain(poolIndex))
    {
        // Initialise the grain directly inside the pool slot to avoid moves.
        grain->channel = channel;
        grain->position = 0;

        const auto lengthMs = juce::jmax(10.0f, grainSizeMs + (randomDist(rng) - 0.5f) * spreadMs);
        grain->length = static_cast<size_t>(millisecondsToSamples(lengthMs, sampleRate));
        grain->length = std::max<std::size_t>(static_cast<std::size_t>(32), grain->length);

        const auto jitterAmount = (randomDist(rng) - 0.5f) * pitchJitter;
        grain->rate = semitoneToRate(pitch + jitterAmount);
        grain->envelope = 0.0f;
        grain->envelopeIncrement = 1.0f / static_cast<float>(grain->length);
        grain->fractionalPosition = 0.0f;
        grain->pan = juce::jlimit(0.0f, 1.0f, randomDist(rng));
        grain->startOffset = scatterSamples > 0 ? static_cast<int>(randomDist(rng) * static_cast<float>(scatterSamples)) : 0;
        grain->active = true;
    }
}

void GrainEngine::updateVisualSnapshot()
{
    auto nextIndex = 1 - visualSnapshotIndex.load(std::memory_order_relaxed);
    auto& snapshot = visualSnapshots[nextIndex];
    snapshot.grainCount = 0;
    snapshot.activeGrains = activeGrainCount;
    snapshot.spawnRatePerSecond = (spawnIntervalSamples > 0.0f && std::isfinite(spawnIntervalSamples))
        ? static_cast<float>(sampleRate) / spawnIntervalSamples
        : 0.0f;
    snapshot.delayTimeMs = delayMs;

    const size_t limit = juce::jmin(activeGrainCount, snapshot.grains.size());
    size_t outIndex = 0;

    for (size_t i = 0; i < activeGrainCount && outIndex < limit; ++i)
    {
        auto poolIndex = activeIndices[i];
        const auto& grain = grainPool[poolIndex];

        if (!grain.active || grain.length == 0)
            continue;

        auto& visual = snapshot.grains[outIndex++];
        visual.pan = grain.pan;
        visual.age = grain.length > 0 ? juce::jlimit(0.0f, 1.0f, static_cast<float>(grain.position) / static_cast<float>(grain.length)) : 0.0f;
        visual.durationSeconds = static_cast<float>(grain.length) / static_cast<float>(sampleRate);
        visual.pitchSemitone = static_cast<float>(std::log2(juce::jmax(0.0001f, grain.rate)) * 12.0f);
        visual.envelope = juce::jlimit(0.0f, 1.0f, grain.envelope);
    }

    snapshot.grainCount = outIndex;
    visualSnapshotIndex.store(nextIndex, std::memory_order_release);
}

GrainEngine::VisualSnapshot GrainEngine::getVisualSnapshot() const
{
    auto index = visualSnapshotIndex.load(std::memory_order_acquire);
    return visualSnapshots[index];
}

float GrainEngine::getWindowValue(float env) const
{
    auto t = juce::jlimit(0.0f, 1.0f, env);
    // Clamp the window to avoid tiny negative values from sin() that would turn into NaNs when pow() is fed a fractional exponent.
    auto base = juce::jlimit(0.0f, 1.0f, std::sin(t * juce::MathConstants<float>::pi));
    if (base <= 0.0f)
        return 0.0f;
    auto exponent = juce::jmap(envelopeShape, 0.0f, 1.0f, 0.5f, 4.0f);
    return std::pow(base, exponent);
}
