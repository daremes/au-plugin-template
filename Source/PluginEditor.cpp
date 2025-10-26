#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <cmath>

namespace
{
constexpr float degToRad(float degrees) { return degrees * juce::MathConstants<float>::pi / 180.0f; }
}

CosmicLookAndFeel::CosmicLookAndFeel()
{
    knobGradient = juce::ColourGradient(juce::Colour(0xff8e7cff), juce::Point<float>(0.0f, 0.0f),
                                        juce::Colour(0xff3ec5ff), juce::Point<float>(0.0f, 40.0f), true);
    knobGradient.addColour(0.5, juce::Colour(0xfff8e1ff));
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void CosmicLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)).reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centre = bounds.getCentre();
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.strokePath(backgroundArc, juce::PathStrokeType(2.0f));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, toAngle, true);
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.strokePath(valueArc, juce::PathStrokeType(2.5f));

    juce::Path knob;
    knob.addEllipse(centre.x - radius * 0.7f, centre.y - radius * 0.7f, radius * 1.4f, radius * 1.4f);
    g.setGradientFill(knobGradient);
    g.fillPath(knob);

    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.drawEllipse(knob.getBounds(), 1.2f);

    auto pointerLength = radius * 0.75f;
    auto pointerThickness = 3.0f;
    juce::Path pointer;
    pointer.addRectangle(-pointerThickness * 0.5f, -radius * 0.05f, pointerThickness, pointerLength);
    g.setColour(juce::Colours::white);
    g.fillPath(pointer, juce::AffineTransform::rotation(toAngle).translated(centre.x, centre.y));

    auto glitchColour = juce::Colour(0xfffefefe).withAlpha(0.12f);
    g.setColour(glitchColour);
    for (int i = 0; i < 3; ++i)
    {
        auto angle = toAngle + degToRad((float) juce::Random::getSystemRandom().nextFloat() * 10.0f - 5.0f);
        auto glitchLen = radius * (0.4f + juce::Random::getSystemRandom().nextFloat() * 0.4f);
        auto start = centre + juce::Point<float>(std::cos(angle), std::sin(angle)) * (radius * 0.2f);
        auto end = start + juce::Point<float>(std::cos(angle), std::sin(angle)) * glitchLen;
        g.drawLine(juce::Line<float>(start, end), 1.0f);
    }

    juce::ignoreUnused(slider);
}

void CosmicLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused(sliderPos, minSliderPos, maxSliderPos, style);
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colour(0xff3ec5ff));
    auto proportion = slider.valueToProportionOfLength(slider.getValue());
    auto fill = bounds;
    fill.setWidth(bounds.getWidth() * proportion);
    g.fillRoundedRectangle(fill, 4.0f);
}

juce::Label* CosmicLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* l = juce::LookAndFeel_V4::createSliderTextBox(slider);
    l->setJustificationType(juce::Justification::centred);
    l->setFont(juce::Font(12.0f));
    l->setColour(juce::Label::textColourId, juce::Colours::white);
    l->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return l;
}

CosmicGrainDelayAudioProcessorEditor::CosmicGrainDelayAudioProcessorEditor(CosmicGrainDelayAudioProcessor& p,
                                                                           juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p), parameters(vts)
{
    initialiseControls();
    startTimerHz(30);
    setSize(720, 520);
    generateStarField();
}

CosmicGrainDelayAudioProcessorEditor::~CosmicGrainDelayAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void CosmicGrainDelayAudioProcessorEditor::initialiseControls()
{
    sliderLabels.clear();
    sliderLabelPairs.clear();

    auto configureSlider = [this](juce::Slider& slider, const juce::String& paramID)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        slider.setLookAndFeel(&lookAndFeel);
        slider.setName(parameters.getParameter(paramID)->getName(32));
        addAndMakeVisible(slider);

        auto label = std::make_unique<juce::Label>();
        label->setText(slider.getName().toUpperCase(), juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::Font(12.0f, juce::Font::bold));
        label->setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.85f));
        addAndMakeVisible(label.get());
        sliderLabelPairs.emplace_back(&slider, label.get());
        sliderLabels.push_back(std::move(label));
    };

    configureSlider(grainSizeSlider, "grainSize");
    configureSlider(densitySlider, "density");
    configureSlider(pitchSlider, "pitch");
    configureSlider(spreadSlider, "spread");
    configureSlider(delaySlider, "delayTime");
    configureSlider(feedbackSlider, "feedback");
    configureSlider(grainWetSlider, "grainWet");
    configureSlider(reverbMixSlider, "reverbMix");
    configureSlider(reverbSizeSlider, "reverbSize");
    configureSlider(reverbDampingSlider, "reverbDamping");
    configureSlider(reverbWidthSlider, "reverbWidth");

    freezeButton.setLookAndFeel(&lookAndFeel);
    freezeButton.setButtonText("Freeze Space");
    freezeButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible(freezeButton);

    grainSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "grainSize", grainSizeSlider);
    densityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "density", densitySlider);
    pitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "pitch", pitchSlider);
    spreadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "spread", spreadSlider);
    delayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "delayTime", delaySlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "feedback", feedbackSlider);
    grainWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "grainWet", grainWetSlider);
    reverbMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "reverbMix", reverbMixSlider);
    reverbSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "reverbSize", reverbSizeSlider);
    reverbDampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "reverbDamping", reverbDampingSlider);
    reverbWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "reverbWidth", reverbWidthSlider);
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(parameters, "reverbFreeze", freezeButton);
}

void CosmicGrainDelayAudioProcessorEditor::generateStarField()
{
    stars.clear();
    auto area = getLocalBounds().toFloat();
    for (int i = 0; i < 140; ++i)
    {
        Star star;
        star.position = { random.nextFloat() * area.getWidth(), random.nextFloat() * area.getHeight() };
        star.radius = juce::jmap(random.nextFloat(), 0.0f, 1.0f, 0.6f, 2.8f);
        star.twinkleSpeed = juce::jmap(random.nextFloat(), 0.0f, 1.0f, 1.2f, 3.5f);
        star.phase = random.nextFloat() * juce::MathConstants<float>::twoPi;
        stars.push_back(star);
    }
}

void CosmicGrainDelayAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient spaceGradient(juce::Colour(0xff0d0221), bounds.getTopLeft(), juce::Colour(0xff1b1f3b), bounds.getBottomRight(), false);
    spaceGradient.addColour(0.3, juce::Colour(0xff332e59));
    spaceGradient.addColour(0.9, juce::Colour(0xff0f3057));
    g.setGradientFill(spaceGradient);
    g.fillRect(bounds);

    for (auto& star : stars)
    {
        auto twinkle = 0.5f + 0.5f * std::sin(star.phase);
        auto colour = juce::Colour::fromHSV(0.65f + 0.05f * twinkle, 0.6f, 0.9f, 0.6f + twinkle * 0.3f);
        g.setColour(colour);
        g.fillEllipse(star.position.x, star.position.y, star.radius, star.radius);
    }

    auto glitchLayer = bounds.reduced(juce::jmap(std::sin(juce::Time::getMillisecondCounter() * 0.002f), -4.0f, 4.0f));
    g.setColour(glitchColour);
    for (int i = 0; i < 40; ++i)
    {
        auto lineY = random.nextFloat() * glitchLayer.getHeight() + glitchLayer.getY();
        auto lineX = glitchLayer.getX() + random.nextFloat() * glitchLayer.getWidth();
        auto length = random.nextFloat() * 120.0f;
        g.fillRect(juce::Rectangle<float>(lineX, lineY, length, 1.0f));
    }

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font("Futura", 28.0f, juce::Font::bold));
    g.drawText("COSMIC GRAIN DELAY", bounds.removeFromTop(50.0f), juce::Justification::centredTop, true);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::italic));
    g.drawText("space // glitch // time", bounds.removeFromTop(30.0f).translated(0, 10), juce::Justification::centredTop, false);

    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawRoundedRectangle(getLocalBounds().reduced(12).toFloat(), 12.0f, 1.5f);
}

void CosmicGrainDelayAudioProcessorEditor::resized()
{
    generateStarField();
    layoutControls();
}

void CosmicGrainDelayAudioProcessorEditor::layoutControls()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(80);

    auto row = area.removeFromTop(140);
    auto cellWidth = row.getWidth() / 4;
    auto placeKnob = [this](juce::Rectangle<int> space, juce::Slider& slider)
    {
        auto knobArea = space.reduced(20);
        slider.setBounds(knobArea);
        for (auto& pair : sliderLabelPairs)
        {
            if (pair.first == &slider)
            {
                auto labelBounds = knobArea.translated(0, -24);
                labelBounds.setHeight(20);
                pair.second->setBounds(labelBounds);
                break;
            }
        }
    };

    placeKnob(row.removeFromLeft(cellWidth), grainSizeSlider);
    placeKnob(row.removeFromLeft(cellWidth), densitySlider);
    placeKnob(row.removeFromLeft(cellWidth), pitchSlider);
    placeKnob(row.removeFromLeft(cellWidth), spreadSlider);

    row = area.removeFromTop(140);
    cellWidth = row.getWidth() / 4;
    placeKnob(row.removeFromLeft(cellWidth), delaySlider);
    placeKnob(row.removeFromLeft(cellWidth), feedbackSlider);
    placeKnob(row.removeFromLeft(cellWidth), grainWetSlider);
    placeKnob(row.removeFromLeft(cellWidth), reverbMixSlider);

    row = area.removeFromTop(140);
    cellWidth = row.getWidth() / 4;
    placeKnob(row.removeFromLeft(cellWidth), reverbSizeSlider);
    placeKnob(row.removeFromLeft(cellWidth), reverbDampingSlider);
    placeKnob(row.removeFromLeft(cellWidth), reverbWidthSlider);

    auto buttonArea = row.removeFromLeft(cellWidth).reduced(20);
    freezeButton.setBounds(buttonArea.removeFromBottom(40));
}

void CosmicGrainDelayAudioProcessorEditor::timerCallback()
{
    for (auto& star : stars)
    {
        star.phase += star.twinkleSpeed * 0.02f;
        if (star.phase > juce::MathConstants<float>::twoPi)
            star.phase -= juce::MathConstants<float>::twoPi;
    }
    repaint();
}
