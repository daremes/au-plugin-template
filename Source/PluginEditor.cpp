#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr int editorWidth = 720;
constexpr int editorHeight = 420;
}

CosmicGlitchAudioProcessorEditor::CosmicGlitchAudioProcessorEditor(CosmicGlitchAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(editorWidth, editorHeight);
    random.setSeedRandomly();

    const auto sliders = {
        &delaySlider, &grainSizeSlider, &densitySlider, &spraySlider,
        &pitchSlider, &feedbackSlider, &textureSlider, &glitchSlider,
        &stereoWidthSlider, &reverbMixSlider, &reverbSizeSlider, &reverbDampingSlider, &wetDrySlider
    };

    for (auto* slider : sliders)
    {
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 18);
        slider->setLookAndFeel(&lookAndFeel);
        addAndMakeVisible(slider);
    }

    addDial(delaySlider, delayLabel, "Delay");
    addDial(grainSizeSlider, grainLabel, "Grain Size");
    addDial(densitySlider, densityLabel, "Density");
    addDial(spraySlider, sprayLabel, "Spray");
    addDial(pitchSlider, pitchLabel, "Pitch");
    addDial(feedbackSlider, feedbackLabel, "Feedback");
    addDial(textureSlider, textureLabel, "Texture");
    addDial(glitchSlider, glitchLabel, "Glitch");
    addDial(stereoWidthSlider, widthLabel, "Width");
    addDial(reverbMixSlider, reverbMixLabel, "Reverb Mix");
    addDial(reverbSizeSlider, reverbSizeLabel, "Reverb Size");
    addDial(reverbDampingSlider, reverbDampingLabel, "Reverb Damp");
    addDial(wetDrySlider, wetDryLabel, "Wet/Dry");

    auto& params = processorRef.getParameters();
    delayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "delay", delaySlider);
    grainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "grainSize", grainSizeSlider);
    densityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "density", densitySlider);
    sprayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "spray", spraySlider);
    pitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "pitch", pitchSlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "feedback", feedbackSlider);
    textureAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "texture", textureSlider);
    glitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "glitch", glitchSlider);
    widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "width", stereoWidthSlider);
    reverbMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "reverbMix", reverbMixSlider);
    reverbSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "reverbSize", reverbSizeSlider);
    reverbDampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "reverbDamping", reverbDampingSlider);
    wetDryAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "wetDry", wetDrySlider);

    startTimerHz(30);
    createBackgroundImage();
}

CosmicGlitchAudioProcessorEditor::~CosmicGlitchAudioProcessorEditor()
{
    stopTimer();

    const auto sliders = {
        &delaySlider, &grainSizeSlider, &densitySlider, &spraySlider,
        &pitchSlider, &feedbackSlider, &textureSlider, &glitchSlider,
        &stereoWidthSlider, &reverbMixSlider, &reverbSizeSlider, &reverbDampingSlider, &wetDrySlider
    };

    for (auto* slider : sliders)
        slider->setLookAndFeel(nullptr);
}

void CosmicGlitchAudioProcessorEditor::paint(juce::Graphics& g)
{
    if (background.isValid())
        g.drawImage(background, getLocalBounds().toFloat());
    else
        g.fillAll(juce::Colour::fromRGB(8, 3, 20));

    juce::Colour neon(180, 255, 250);
    g.setColour(neon.withAlpha(0.9f));
    g.setFont(juce::Font("Orbitron", 32.0f, juce::Font::bold));
    g.drawText("Cosmic Glitch", 24, 18, 300, 40, juce::Justification::left);

    g.setFont(juce::Font(14.0f));
    g.drawText("granular delay • space debris reverb • designed for cosmic noise sculpting", 24, getHeight() - 30, getWidth() - 48, 20, juce::Justification::centred);
}

void CosmicGlitchAudioProcessorEditor::resized()
{
    const int margin = 24;
    const int labelHeight = 20;
    const int sliderSize = 96;
    const int spacing = 18;

    auto bounds = getLocalBounds().reduced(margin);
    bounds.removeFromTop(70);
    auto bottomText = bounds.removeFromBottom(40);
    juce::ignoreUnused(bottomText);

    auto columnWidth = bounds.getWidth() / 3;
    auto column = bounds.removeFromLeft(columnWidth);

    auto placeControl = [&](juce::Slider& slider, juce::Label& label)
    {
        auto area = column.removeFromTop(sliderSize + labelHeight + spacing);
        auto sliderArea = area.removeFromTop(sliderSize);
        slider.setBounds(sliderArea.reduced(6));
        label.setBounds(area);
    };

    placeControl(delaySlider, delayLabel);
    placeControl(grainSizeSlider, grainLabel);
    placeControl(densitySlider, densityLabel);
    placeControl(spraySlider, sprayLabel);

    column = bounds.removeFromLeft(columnWidth);
    placeControl(pitchSlider, pitchLabel);
    placeControl(feedbackSlider, feedbackLabel);
    placeControl(textureSlider, textureLabel);
    placeControl(glitchSlider, glitchLabel);

    column = bounds;
    placeControl(stereoWidthSlider, widthLabel);
    placeControl(reverbMixSlider, reverbMixLabel);
    placeControl(reverbSizeSlider, reverbSizeLabel);
    placeControl(reverbDampingSlider, reverbDampingLabel);
    placeControl(wetDrySlider, wetDryLabel);
}

void CosmicGlitchAudioProcessorEditor::timerCallback()
{
    if (!background.isValid())
        return;

    juce::Graphics g(background);

    juce::Colour glitchColour = juce::Colour::fromRGBA(random.nextInt(256), random.nextInt(256), random.nextInt(256), 35);
    int glitchWidth = random.nextInt(60) + 10;
    int glitchHeight = random.nextInt(20) + 4;
    int x = random.nextInt(background.getWidth());
    int y = random.nextInt(background.getHeight());

    g.setColour(glitchColour);
    g.fillRect(x, y, glitchWidth, glitchHeight);

    repaint();
}

void CosmicGlitchAudioProcessorEditor::createBackgroundImage()
{
    background = juce::Image(juce::Image::RGB, getWidth(), getHeight(), true);
    juce::Graphics g(background);

    juce::ColourGradient spaceGradient(juce::Colour::fromRGB(4, 2, 12), 0, 0,
                                       juce::Colour::fromRGB(22, 8, 36), static_cast<float>(getWidth()), static_cast<float>(getHeight()), false);
    spaceGradient.addColour(0.4f, juce::Colour::fromRGB(10, 4, 24));
    spaceGradient.addColour(0.9f, juce::Colour::fromRGB(60, 20, 80));

    g.setGradientFill(spaceGradient);
    g.fillAll();

    for (int i = 0; i < 160; ++i)
    {
        auto starColour = juce::Colour::fromRGB(200 + random.nextInt(55), 200 + random.nextInt(55), 255);
        auto pos = juce::Point<float>(random.nextFloat() * getWidth(), random.nextFloat() * getHeight());
        float radius = random.nextFloat() * 1.8f + 0.4f;
        g.setColour(starColour.withAlpha(0.7f));
        g.fillEllipse({ pos.x - radius, pos.y - radius, radius * 2.0f, radius * 2.0f });
    }

    for (int streak = 0; streak < 30; ++streak)
    {
        juce::Colour streakColour = juce::Colour::fromRGBA(120, 200, 255, 50 + random.nextInt(50));
        juce::Path streakPath;
        auto start = juce::Point<float>(random.nextFloat() * getWidth(), random.nextFloat() * getHeight());
        auto end = start + juce::Point<float>(random.nextFloat() * 120.0f - 60.0f, random.nextFloat() * 40.0f - 20.0f);
        streakPath.startNewSubPath(start);
        streakPath.lineTo(end);
        g.setColour(streakColour);
        g.strokePath(streakPath, juce::PathStrokeType(2.0f));
    }
}

void CosmicGlitchAudioProcessorEditor::addDial(juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    addAndMakeVisible(label);
    label.setText(name, juce::NotificationType::dontSendNotification);
    label.setFont(juce::Font(13.0f));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
}

