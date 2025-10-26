#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <cmath>

namespace
{
constexpr float degToRad(float degrees) { return degrees * juce::MathConstants<float>::pi / 180.0f; }
}

namespace
{
juce::ColourGradient makeGradientForID(const juce::String& id, const juce::Point<float>& centre, float radius)
{
    auto primary = juce::Colour(0xff3ec5ff);
    auto highlight = juce::Colour(0xff8e7cff);

    if (id == "grain")
    {
        primary = juce::Colour(0xff40d1ff);
        highlight = juce::Colour(0xff7ef4c9);
    }
    else if (id == "delay")
    {
        primary = juce::Colour(0xff5f8bff);
        highlight = juce::Colour(0xffa189ff);
    }
    else if (id == "distortion")
    {
        primary = juce::Colour(0xffff7f5e);
        highlight = juce::Colour(0xffffbf65);
    }
    else if (id == "reverb")
    {
        primary = juce::Colour(0xff6cb9ff);
        highlight = juce::Colour(0xff9fd2ff);
    }

    juce::ColourGradient gradient(primary, centre.x, centre.y - radius * 0.6f,
                                  highlight, centre.x, centre.y + radius * 0.6f, true);
    gradient.addColour(0.5, juce::Colour(0xfff8e1ff).withAlpha(0.9f));
    return gradient;
}
}

CosmicLookAndFeel::CosmicLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ToggleButton::textColourId, juce::Colours::white);
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
    auto gradient = makeGradientForID(slider.getComponentID(), centre, radius);
    g.setGradientFill(gradient);
    g.fillPath(knob);

    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.drawEllipse(knob.getBounds(), 1.2f);

    auto pointerLength = radius * 0.82f;
    auto pointerThickness = juce::jmax(2.0f, radius * 0.08f);
    juce::Path pointer;
    pointer.startNewSubPath(0.0f, -pointerLength);
    pointer.lineTo(pointerThickness * 0.5f, 0.0f);
    pointer.lineTo(-pointerThickness * 0.5f, 0.0f);
    pointer.closeSubPath();
    g.setColour(juce::Colours::white.withAlpha(0.9f));
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

void CosmicLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto trackHeight = juce::jmin(bounds.getHeight(), 20.0f);
    auto trackWidth = juce::jmax(trackHeight * 1.8f, bounds.getWidth());
    auto track = juce::Rectangle<float>(bounds.getCentreX() - trackWidth * 0.5f,
                                        bounds.getCentreY() - trackHeight * 0.5f,
                                        trackWidth,
                                        trackHeight);

    auto baseColour = button.getToggleState() ? juce::Colour(0xff40d1ff) : juce::Colours::white.withAlpha(0.18f);
    if (isHighlighted)
        baseColour = baseColour.brighter(0.25f);
    if (isDown)
        baseColour = baseColour.darker(0.1f);

    g.setColour(baseColour.withAlpha(0.35f));
    g.fillRoundedRectangle(track, trackHeight * 0.5f);
    g.setColour(baseColour.withAlpha(0.65f));
    g.drawRoundedRectangle(track, trackHeight * 0.5f, 1.4f);

    auto thumbDiameter = trackHeight * 0.7f;
    auto thumbPadding = (trackHeight - thumbDiameter) * 0.5f;
    auto thumbX = button.getToggleState()
        ? (track.getRight() - thumbDiameter - thumbPadding)
        : (track.getX() + thumbPadding);
    auto thumbBounds = juce::Rectangle<float>(thumbX, track.getY() + thumbPadding, thumbDiameter, thumbDiameter);

    auto gradient = makeGradientForID(button.getComponentID(), thumbBounds.getCentre(), thumbDiameter * 0.5f);
    g.setGradientFill(gradient);
    g.fillEllipse(thumbBounds);
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawEllipse(thumbBounds, 1.1f);
}

CosmicGrainDelayAudioProcessorEditor::CosmicGrainDelayAudioProcessorEditor(CosmicGrainDelayAudioProcessor& p,
                                                                           juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p), parameters(vts)
{
    initialiseControls();
    startTimerHz(30);
    setSize(1160, 840);
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
    toggleLabels.clear();
    toggleLabelPairs.clear();

    auto configureSlider = [this](juce::Slider& slider, const juce::String& paramID, const juce::String& colourID)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        slider.setLookAndFeel(&lookAndFeel);
        slider.setName(parameters.getParameter(paramID)->getName(32));
        slider.setComponentID(colourID);
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

    configureSlider(grainSizeSlider, "grainSize", "grain");
    configureSlider(densitySlider, "density", "grain");
    configureSlider(pitchSlider, "pitch", "grain");
    configureSlider(spreadSlider, "spread", "grain");
    configureSlider(grainScatterSlider, "grainScatter", "grain");
    configureSlider(grainEnvelopeSlider, "grainEnvelopeShape", "grain");
    configureSlider(grainJitterSlider, "grainPitchJitter", "grain");
    configureSlider(delaySlider, "delayTime", "delay");
    configureSlider(delayDivisionSlider, "delayDivision", "delay");
    configureSlider(feedbackSlider, "feedback", "delay");
    configureSlider(distortionDriveSlider, "distortionDrive", "distortion");
    configureSlider(distortionToneSlider, "distortionTone", "distortion");
    configureSlider(distortionMixSlider, "distortionMix", "distortion");
    configureSlider(grainWetSlider, "grainWet", "grain");
    configureSlider(reverbMixSlider, "reverbMix", "reverb");
    configureSlider(reverbSizeSlider, "reverbSize", "reverb");
    configureSlider(reverbDampingSlider, "reverbDamping", "reverb");
    configureSlider(reverbWidthSlider, "reverbWidth", "reverb");

    delayDivisionSlider.setNumDecimalPlacesToDisplay(0);
    delayDivisionSlider.textFromValueFunction = [](double value)
    {
        auto index = juce::jlimit<int>(0, static_cast<int>(CosmicGrainDelayAudioProcessor::delayDivisionLabels.size() - 1),
            static_cast<int>(std::round(value)));
        return juce::String(CosmicGrainDelayAudioProcessor::delayDivisionLabels[static_cast<size_t>(index)]);
    };
    delayDivisionSlider.valueFromTextFunction = [](const juce::String& text)
    {
        for (size_t i = 0; i < CosmicGrainDelayAudioProcessor::delayDivisionLabels.size(); ++i)
            if (text.equalsIgnoreCase(CosmicGrainDelayAudioProcessor::delayDivisionLabels[i]))
                return static_cast<double>(i);
        return 0.0;
    };

    auto setToggleText = [this](juce::ToggleButton& button, const juce::String& paramID, const juce::String& colourID)
    {
        if (auto* param = parameters.getParameter(paramID))
        {
            auto label = std::make_unique<juce::Label>();
            label->setText(param->getName(32).toUpperCase(), juce::dontSendNotification);
            label->setJustificationType(juce::Justification::centred);
            label->setFont(juce::Font(12.0f, juce::Font::bold));
            label->setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.85f));
            addAndMakeVisible(label.get());
            toggleLabelPairs.emplace_back(&button, label.get());
            toggleLabels.push_back(std::move(label));
        }
        button.setButtonText({});
        button.setComponentID(colourID);
        button.setLookAndFeel(&lookAndFeel);
        addAndMakeVisible(button);
    };

    setToggleText(delaySyncButton, "delaySync", "delay");
    setToggleText(distortionToggle, "distortionEnabled", "distortion");
    freezeButton.setLookAndFeel(&lookAndFeel);
    setToggleText(freezeButton, "reverbFreeze", "reverb");

    grainSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "grainSize", grainSizeSlider);
    densityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "density", densitySlider);
    pitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "pitch", pitchSlider);
    spreadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "spread", spreadSlider);
    grainScatterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "grainScatter", grainScatterSlider);
    grainEnvelopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "grainEnvelopeShape", grainEnvelopeSlider);
    grainJitterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "grainPitchJitter", grainJitterSlider);
    delayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "delayTime", delaySlider);
    delayDivisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "delayDivision", delayDivisionSlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "feedback", feedbackSlider);
    distortionDriveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "distortionDrive", distortionDriveSlider);
    distortionToneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "distortionTone", distortionToneSlider);
    distortionMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "distortionMix", distortionMixSlider);
    grainWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "grainWet", grainWetSlider);
    reverbMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "reverbMix", reverbMixSlider);
    reverbSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "reverbSize", reverbSizeSlider);
    reverbDampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "reverbDamping", reverbDampingSlider);
    reverbWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "reverbWidth", reverbWidthSlider);
    delaySyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(parameters, "delaySync", delaySyncButton);
    distortionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(parameters, "distortionEnabled", distortionToggle);
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(parameters, "reverbFreeze", freezeButton);

    delaySyncButton.onStateChange = [this]
    {
        delayDivisionSlider.setEnabled(delaySyncButton.getToggleState());
    };

    distortionToggle.onStateChange = [this]
    {
        const auto enabled = distortionToggle.getToggleState();
        distortionDriveSlider.setEnabled(enabled);
        distortionToneSlider.setEnabled(enabled);
        distortionMixSlider.setEnabled(enabled);
    };

    auto findLabel = [this](juce::Slider& slider) -> juce::Label*
    {
        for (auto& pair : sliderLabelPairs)
            if (pair.first == &slider)
                return pair.second;
        return nullptr;
    };

    auto updateDelayMode = [this, findLabel]()
    {
        const bool sync = delaySyncButton.getToggleState();
        delayDivisionSlider.setVisible(sync);
        delayDivisionSlider.setEnabled(sync);
        if (auto* label = findLabel(delayDivisionSlider))
            label->setVisible(sync);

        delaySlider.setVisible(!sync);
        delaySlider.setEnabled(!sync);
        if (auto* label = findLabel(delaySlider))
            label->setVisible(!sync);
    };

    auto setDistortionEnabled = [this, findLabel](bool enabled)
    {
        auto apply = [&](juce::Slider& slider)
        {
            slider.setEnabled(enabled);
            slider.setAlpha(enabled ? 1.0f : 0.35f);
            if (auto* label = findLabel(slider))
                label->setAlpha(enabled ? 1.0f : 0.35f);
        };

        apply(distortionDriveSlider);
        apply(distortionToneSlider);
        apply(distortionMixSlider);
    };

    delaySyncButton.onStateChange = [updateDelayMode]() mutable
    {
        updateDelayMode();
    };

    distortionToggle.onStateChange = [setDistortionEnabled, this]() mutable
    {
        setDistortionEnabled(distortionToggle.getToggleState());
    };

    updateDelayMode();
    setDistortionEnabled(distortionToggle.getToggleState());
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
    auto brandingArea = bounds.removeFromTop(70.0f);
    auto brandLine = brandingArea.removeFromTop(24.0f);
    g.setFont(juce::Font("Futura", 18.0f, juce::Font::bold));
    g.drawText("FELINE ASTRONAUTS", brandLine, juce::Justification::centredTop, true);
    g.setFont(juce::Font("Futura", 30.0f, juce::Font::bold));
    g.drawText("COSMIC SCRATCHES", brandingArea, juce::Justification::centredTop, true);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::italic));
    g.drawText("claws // nebulae // texture", bounds.removeFromTop(30.0f).translated(0, 10), juce::Justification::centredTop, false);

    if (!grainVisualiserBounds.isEmpty())
    {
        auto visualiser = grainVisualiserBounds.toFloat();
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(visualiser, 18.0f);
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(visualiser, 18.0f, 1.6f);

        auto dashedColour = juce::Colours::white.withAlpha(0.15f);
        g.setColour(dashedColour);
        for (int i = 0; i < 6; ++i)
        {
            auto orbitPhase = static_cast<double>(i) + juce::Time::getMillisecondCounterHiRes() * 0.001;
            auto wave = static_cast<float>(0.5 + 0.5 * std::sin(orbitPhase));
            g.setColour(dashedColour.withAlpha(wave * 0.35f));
            auto orbitRadius = visualiser.getWidth() * (0.18f + 0.12f * static_cast<float>(i));
            auto orbitBounds = juce::Rectangle<float>(orbitRadius, orbitRadius);
            orbitBounds = orbitBounds.withCentre(visualiser.getCentre());
            g.drawEllipse(orbitBounds, 0.8f);
        }

        auto centre = visualiser.getCentre();
        auto maxRadius = juce::jmin(visualiser.getWidth(), visualiser.getHeight()) * 0.45f;
        auto innerRadius = juce::jmin(visualiser.getWidth(), visualiser.getHeight()) * 0.18f;
        auto now = juce::Time::getMillisecondCounterHiRes() * 0.001;

        if (latestSnapshot.grainCount == 0)
        {
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.setFont(juce::Font(14.0f, juce::Font::italic));
            g.drawText("waiting for grains", grainVisualiserBounds, juce::Justification::centred);
        }
        else
        {
            auto tailColour = juce::Colours::white.withAlpha(0.2f);
            for (size_t i = 0; i < latestSnapshot.grainCount; ++i)
            {
                const auto& grain = latestSnapshot.grains[i];

                auto progress = juce::jlimit(0.0f, 1.0f, grain.age);
                auto pitchHue = juce::jlimit(0.0f, 1.0f, 0.55f + grain.pitchSemitone * 0.015f);
                auto size = juce::jlimit(6.0f, 20.0f, 8.0f + grain.durationSeconds * 60.0f);
                auto energy = juce::jlimit(0.2f, 1.0f, 0.3f + grain.envelope);

                auto rotation = static_cast<float>(now * 0.35) + grain.pan * juce::MathConstants<float>::twoPi;
                auto radius = innerRadius + (maxRadius - innerRadius) * progress;
                auto position = centre + juce::Point<float>(std::cos(rotation), std::sin(rotation)) * radius;

                auto particleColour = juce::Colour::fromHSV(pitchHue, 0.6f, 0.9f, energy);
                g.setColour(tailColour.withAlpha(energy * 0.6f));
                g.drawLine({ centre, position }, 1.0f);

                g.setColour(particleColour);
                g.fillEllipse({ position.x - size * 0.5f, position.y - size * 0.5f, size, size });
            }

            g.setColour(juce::Colours::white.withAlpha(0.55f));
            g.setFont(juce::Font(12.0f, juce::Font::plain));
            juce::String telemetry;
            telemetry << juce::String(latestSnapshot.activeGrains) << " grains   |   "
                      << juce::String(juce::roundToInt(latestSnapshot.spawnRatePerSecond)) << " grains/sec   |   "
                      << juce::String(latestSnapshot.delayTimeMs, 1) << " ms delay";
            g.drawFittedText(telemetry, grainVisualiserBounds.reduced(12, 8), juce::Justification::topLeft, 1);
        }
    }

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
    auto bounds = getLocalBounds().reduced(48);
    bounds.removeFromTop(140);

    auto positionSliderLabel = [this](juce::Slider& slider, const juce::Rectangle<int>& controlBounds)
    {
        for (auto& pair : sliderLabelPairs)
        {
            if (pair.first == &slider)
            {
                auto labelBounds = controlBounds.withHeight(20);
                labelBounds.setY(controlBounds.getY() - 24);
                pair.second->setBounds(labelBounds);
                break;
            }
        }
    };

    auto positionToggleLabel = [this](juce::ToggleButton& button, const juce::Rectangle<int>& labelBounds)
    {
        for (auto& pair : toggleLabelPairs)
        {
            if (pair.first == &button)
            {
                pair.second->setBounds(labelBounds);
                break;
            }
        }
    };

    auto layoutSlider = [&](juce::Slider& slider, const juce::Rectangle<int>& area)
    {
        auto knobBounds = area;
        auto size = juce::jmin(knobBounds.getWidth(), knobBounds.getHeight());
        size = juce::jmax(size - 12, 96);
        knobBounds = juce::Rectangle<int>(0, 0, size, size);
        knobBounds.setCentre(area.getCentre());
        slider.setBounds(knobBounds);
        positionSliderLabel(slider, knobBounds);
    };

    const int knobRowHeight = 150;

    auto layoutSliderGrid = [&](const std::vector<juce::Slider*>& sliders, juce::Rectangle<int> area, int columns)
    {
        if (sliders.empty())
            return;

        auto working = area.reduced(4);
        const int total = static_cast<int>(sliders.size());
        int index = 0;

        while (index < total && working.getHeight() > 0)
        {
            auto rowArea = working.removeFromTop(knobRowHeight);
            const int cellWidth = rowArea.getWidth() / columns;

            for (int col = 0; col < columns && index < total; ++col)
            {
                auto cell = juce::Rectangle<int>(rowArea.getX() + col * cellWidth,
                                                 rowArea.getY(),
                                                 cellWidth,
                                                 rowArea.getHeight()).reduced(12);
                layoutSlider(*sliders[static_cast<size_t>(index)], cell);
                ++index;
            }
        }
    };

    auto layoutToggle = [&](juce::ToggleButton& button, juce::Rectangle<int> area)
    {
        const int toggleWidth = juce::jmin(area.getWidth(), 72);
        const int toggleHeight = 26;
        auto toggleBounds = juce::Rectangle<int>(toggleWidth, toggleHeight).withCentre(area.getCentre());
        button.setBounds(toggleBounds);
        auto labelArea = toggleBounds.expanded(0, 12);
        labelArea.setHeight(20);
        labelArea.setY(toggleBounds.getY() - 26);
        positionToggleLabel(button, labelArea);
    };

    auto workingArea = bounds.reduced(24);
    const int visualiserHeight = juce::jlimit(160, 240, workingArea.getHeight() / 3);
    grainVisualiserBounds = workingArea.removeFromBottom(visualiserHeight).reduced(20);

    auto controlArea = workingArea.reduced(12);
    const int columnSpacing = 32;

    auto grainColumn = controlArea.removeFromLeft(static_cast<int>(controlArea.getWidth() * 0.38f));
    controlArea.removeFromLeft(columnSpacing);
    auto timeColumn = controlArea.removeFromLeft(static_cast<int>(controlArea.getWidth() * 0.30f));
    controlArea.removeFromLeft(columnSpacing);
    auto fxColumn = controlArea;

    grainColumn = grainColumn.reduced(8);
    timeColumn = timeColumn.reduced(8);
    fxColumn = fxColumn.reduced(8);

    layoutSliderGrid({ &grainSizeSlider, &densitySlider, &pitchSlider, &spreadSlider,
                       &grainScatterSlider, &grainEnvelopeSlider, &grainJitterSlider, &grainWetSlider },
                     grainColumn, 2);

    auto timeArea = timeColumn;
    auto syncArea = timeArea.removeFromTop(40);
    layoutToggle(delaySyncButton, syncArea);
    timeArea.removeFromTop(12);

    auto delayArea = timeArea.removeFromTop(knobRowHeight);
    layoutSlider(delaySlider, delayArea);
    layoutSlider(delayDivisionSlider, delayArea);

    timeArea.removeFromTop(12);
    auto feedbackArea = timeArea.removeFromTop(knobRowHeight);
    layoutSlider(feedbackSlider, feedbackArea);

    auto fxArea = fxColumn;
    auto distortionArea = fxArea.removeFromTop(knobRowHeight + 24);
    auto distortionToggleArea = distortionArea.removeFromTop(38);
    layoutToggle(distortionToggle, distortionToggleArea);
    distortionArea.removeFromTop(8);
    layoutSliderGrid({ &distortionDriveSlider, &distortionToneSlider, &distortionMixSlider }, distortionArea, 3);

    fxArea.removeFromTop(20);
    auto freezeArea = fxArea.removeFromBottom(48);
    layoutToggle(freezeButton, freezeArea);
    fxArea.removeFromBottom(8);
    layoutSliderGrid({ &reverbMixSlider, &reverbSizeSlider, &reverbDampingSlider, &reverbWidthSlider }, fxArea, 2);
}

void CosmicGrainDelayAudioProcessorEditor::timerCallback()
{
    latestSnapshot = audioProcessor.getGrainVisualSnapshot();
    for (auto& star : stars)
    {
        star.phase += star.twinkleSpeed * 0.02f;
        if (star.phase > juce::MathConstants<float>::twoPi)
            star.phase -= juce::MathConstants<float>::twoPi;
    }
    repaint();
}
