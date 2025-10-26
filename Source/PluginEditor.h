#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include <memory>

#include "GlitchLookAndFeel.h"
#include "PluginProcessor.h"

class CosmicGlitchAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit CosmicGlitchAudioProcessorEditor(CosmicGlitchAudioProcessor&);
    ~CosmicGlitchAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void createBackgroundImage();
    void addDial(juce::Slider& slider, juce::Label& label, const juce::String& name);

    CosmicGlitchAudioProcessor& processorRef;
    GlitchLookAndFeel lookAndFeel;

    juce::Image background;
    juce::Random random;

    juce::Slider delaySlider;
    juce::Slider grainSizeSlider;
    juce::Slider densitySlider;
    juce::Slider spraySlider;
    juce::Slider pitchSlider;
    juce::Slider feedbackSlider;
    juce::Slider textureSlider;
    juce::Slider glitchSlider;
    juce::Slider stereoWidthSlider;
    juce::Slider reverbMixSlider;
    juce::Slider reverbSizeSlider;
    juce::Slider reverbDampingSlider;
    juce::Slider wetDrySlider;

    juce::Label delayLabel;
    juce::Label grainLabel;
    juce::Label densityLabel;
    juce::Label sprayLabel;
    juce::Label pitchLabel;
    juce::Label feedbackLabel;
    juce::Label textureLabel;
    juce::Label glitchLabel;
    juce::Label widthLabel;
    juce::Label reverbMixLabel;
    juce::Label reverbSizeLabel;
    juce::Label reverbDampingLabel;
    juce::Label wetDryLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> densityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sprayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> textureAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbDampingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetDryAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CosmicGlitchAudioProcessorEditor)
};

