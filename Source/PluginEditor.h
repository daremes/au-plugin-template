#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <memory>
#include <utility>
#include <vector>

class CosmicGrainDelayAudioProcessor;

class CosmicLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CosmicLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                          float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Label* createSliderTextBox(juce::Slider& slider) override;

private:
    juce::ColourGradient knobGradient;
};

class CosmicGrainDelayAudioProcessorEditor : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    CosmicGrainDelayAudioProcessorEditor(CosmicGrainDelayAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~CosmicGrainDelayAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void initialiseControls();
    void layoutControls();
    void generateStarField();

    CosmicGrainDelayAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& parameters;

    CosmicLookAndFeel lookAndFeel;
    juce::Random random;

    juce::Slider grainSizeSlider;
    juce::Slider densitySlider;
    juce::Slider pitchSlider;
    juce::Slider spreadSlider;
    juce::Slider delaySlider;
    juce::Slider feedbackSlider;
    juce::Slider grainWetSlider;
    juce::Slider reverbMixSlider;
    juce::Slider reverbSizeSlider;
    juce::Slider reverbDampingSlider;
    juce::Slider reverbWidthSlider;
    juce::ToggleButton freezeButton { "Freeze" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> densityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> spreadAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbDampingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;

    std::vector<std::unique_ptr<juce::Label>> sliderLabels;
    std::vector<std::pair<juce::Slider*, juce::Label*>> sliderLabelPairs;

    struct Star
    {
        juce::Point<float> position;
        float radius = 1.0f;
        float twinkleSpeed = 1.0f;
        float phase = 0.0f;
    };

    std::vector<Star> stars;
    juce::Colour glitchColour { juce::Colours::white.withAlpha(0.08f) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CosmicGrainDelayAudioProcessorEditor)
};
