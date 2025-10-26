#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>

class GlitchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GlitchLookAndFeel()
    {
        using namespace juce;
        setColour(Slider::thumbColourId, Colour::fromRGB(212, 120, 255));
        setColour(Slider::trackColourId, Colour::fromRGB(40, 14, 70));
        setColour(Slider::rotarySliderFillColourId, Colour::fromRGB(120, 212, 255));
        setColour(Slider::rotarySliderOutlineColourId, Colour::fromRGB(18, 6, 33));
        setColour(Label::textColourId, Colours::white.withAlpha(0.85f));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override
    {
        using namespace juce;

        const auto bounds = Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)).reduced(6.0f);
        const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        const auto centre = bounds.getCentre();
        const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        Colour gradientStart(32, 18, 64);
        Colour gradientEnd(118, 54, 181);
        Colour neon = Colour::fromRGB(180, 255, 250);

        ColourGradient gradient(gradientStart, bounds.getX(), bounds.getY(),
                                 gradientEnd, bounds.getRight(), bounds.getBottom(), false);
        gradient.addColour(0.5f, Colour::fromRGB(44, 12, 96));
        g.setGradientFill(gradient);
        g.fillEllipse(bounds);

        g.setColour(Colour::fromRGB(12, 4, 28));
        g.drawEllipse(bounds, 2.5f);

        const float indicatorLength = radius * 0.7f;
        const float indicatorThickness = 3.0f;
        juce::Path indicator;
        indicator.startNewSubPath(centre.getX(), centre.getY());
        indicator.lineTo(centre.getX() + std::cos(angle) * indicatorLength,
                         centre.getY() + std::sin(angle) * indicatorLength);
        g.setColour(neon);
        g.strokePath(indicator, PathStrokeType(indicatorThickness, PathStrokeType::mitered, PathStrokeType::rounded));

        if (auto* textBox = slider.getTextBoxComponent())
        {
            textBox->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
            textBox->setColour(TextEditor::textColourId, Colours::white);
            textBox->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        }
    }
};

