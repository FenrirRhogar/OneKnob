/*
  ==============================================================================

    PluginEditor.cpp
    OneKnobToRuleThemAll — SVG Asset based GUI.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Required for binary assets
#include "BinaryData.h"

//==============================================================================
// Colour palette helpers
//==============================================================================
juce::Colour OneKnobLookAndFeel::getArcColour (float normalised)
{
    // Fiery One Ring Gradient: Gold (0.15) to Deep Red (0.0)
    float hue = 0.15f - (normalised * 0.15f);
    if (hue < 0.0f) hue += 1.0f;
    return juce::Colour::fromHSV (hue, 0.9f, 0.95f, 1.0f);
}

//==============================================================================
// Custom LookAndFeel
//==============================================================================
OneKnobLookAndFeel::OneKnobLookAndFeel()
{
    // Load PNG
    knobImg = juce::ImageCache::getFromMemory (BinaryData::Knob_png, BinaryData::Knob_pngSize);

    // Modern Dark Theme Defaults for ComboBox
    setColour (juce::ComboBox::backgroundColourId,        juce::Colour (0xFF1E1F24));
    setColour (juce::ComboBox::textColourId,              juce::Colour (0xFFF0F0F0));
    setColour (juce::ComboBox::outlineColourId,           juce::Colour (0xFF33353D));
    setColour (juce::ComboBox::arrowColourId,             juce::Colour (0xFFA0A0A0));
    
    setColour (juce::PopupMenu::backgroundColourId,       juce::Colour (0xFF23252B));
    setColour (juce::PopupMenu::textColourId,             juce::Colour (0xFFF0F0F0));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xFF3F424E));
    setColour (juce::PopupMenu::highlightedTextColourId,  juce::Colour (0xFFFFFFFF));
}

void OneKnobLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                           int x, int y, int width, int height,
                                           float sliderPos,
                                           float rotaryStartAngle,
                                           float rotaryEndAngle,
                                           juce::Slider& /*slider*/)
{
    const auto bounds   = juce::Rectangle<float> (static_cast<float>(x), static_cast<float>(y),
                                                  static_cast<float>(width), static_cast<float>(height));
    const auto maxRadius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto radius   = maxRadius - 40.0f; // Increased padding to prevent clipping the outer glow
    const auto centreX  = bounds.getCentreX();
    const auto centreY  = bounds.getCentreY();
    const auto diameter = radius * 2.0f;
    const auto angle    = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const juce::Colour currentColour = getArcColour (sliderPos);

    // ---- Dynamic Outer Glow -------------------------------------------------
    {
        const float glowBase = 12.0f;
        const float glowExtra = 20.0f * sliderPos;
        const float glowRadius = radius + glowBase + glowExtra;
        
        juce::ColourGradient glow (currentColour.withAlpha (0.25f + 0.15f * sliderPos), centreX, centreY,
                                   currentColour.withAlpha (0.0f), centreX - glowRadius, centreY - glowRadius,
                                   true);
        g.setGradientFill (glow);
        g.fillEllipse (centreX - glowRadius, centreY - glowRadius,
                       glowRadius * 2.0f, glowRadius * 2.0f);
    }

    // ---- Track arc (dim grey) -----------------------------------------------
    {
        const float trackThickness = 12.0f; // THICKER TRACK
        juce::Path trackArc;
        trackArc.addCentredArc (centreX, centreY, radius + 10.0f, radius + 10.0f,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xFF26282E));
        g.strokePath (trackArc, juce::PathStrokeType (trackThickness,
                      juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ---- Filled arc (premium gradient) behind the knob ----------------------
    if (sliderPos > 0.001f)
    {
        const float fillThickness = 12.0f; // THICKER FILL
        juce::Path filledArc;
        filledArc.addCentredArc (centreX, centreY, radius + 10.0f, radius + 10.0f,
                                 0.0f, rotaryStartAngle, angle, true);

        juce::ColourGradient grad (getArcColour(0.0f), centreX - radius, centreY + radius,
                                   currentColour, centreX + radius, centreY - radius,
                                   false);
        g.setGradientFill (grad);
        g.strokePath (filledArc, juce::PathStrokeType (fillThickness,
                      juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ---- Draw Custom Knob PNG ----------------------------------------------
    juce::Rectangle<float> imgBounds (centreX - radius, centreY - radius, diameter, diameter);

    if (knobImg.isValid())
    {
        g.addTransform (juce::AffineTransform::rotation (angle, centreX, centreY));
        g.drawImage (knobImg, imgBounds);
        g.addTransform (juce::AffineTransform::rotation (-angle, centreX, centreY));
    }
    else
    {
        g.setColour (juce::Colours::black);
        g.fillEllipse (imgBounds);
    }
}

void OneKnobLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                           const juce::Slider::SliderStyle /*style*/, juce::Slider& /*slider*/)
{
    juce::Rectangle<float> track (static_cast<float>(x), y + height * 0.35f, static_cast<float>(width), height * 0.3f);
    
    // Draw track
    g.setColour (juce::Colour (0xFF1E1F24));
    g.fillRoundedRectangle (track, track.getHeight() * 0.5f);

    g.setColour (juce::Colour (0xFF33353D));
    g.drawRoundedRectangle (track, track.getHeight() * 0.5f, 1.5f);

    // Draw Thumb
    float thumbWidth = height * 0.9f;
    juce::Rectangle<float> thumb (sliderPos - thumbWidth * 0.5f, y + (height - thumbWidth) * 0.5f, thumbWidth, thumbWidth);
    
    g.setColour (juce::Colour (0xFFFCA311)); // Gold
    g.fillEllipse (thumb);
    
    g.setColour (juce::Colours::black);
    g.drawEllipse (thumb, 1.5f);
}


//==============================================================================
// Editor
//==============================================================================
OneKnobToRuleThemAllAudioProcessorEditor::OneKnobToRuleThemAllAudioProcessorEditor (
    OneKnobToRuleThemAllAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&oneKnobLookAndFeel);

    // Load Background PNG
    bgImg = juce::ImageCache::getFromMemory (BinaryData::Background_png, BinaryData::Background_pngSize);

    // ---- Drive slider -------------------------------------------------------
    driveSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    driveSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    driveSlider.setPopupDisplayEnabled (false, false, this); 
    addAndMakeVisible (driveSlider);

    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "macroDrive", driveSlider);

    driveSlider.onValueChange = [this] { cachedDrive = static_cast<float>(driveSlider.getValue()); };

    // ---- Flavor Switch (3-Way Toggle) ---------------------------------------
    modeSwitch.setSliderStyle (juce::Slider::LinearHorizontal);
    modeSwitch.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    modeSwitch.setRange (0, 2, 1);
    addAndMakeVisible (modeSwitch);

    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "satFlavor", modeSwitch);

    modeSwitch.onValueChange = [this] { repaint(); };

    // ---- Window size --------------------------------------------------------
    setSize (360, 540);
    
    startTimerHz(30);
}

OneKnobToRuleThemAllAudioProcessorEditor::~OneKnobToRuleThemAllAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void OneKnobToRuleThemAllAudioProcessorEditor::timerCallback()
{
    static float lastDrive = -1.0f;
    if (std::abs(cachedDrive - lastDrive) > 0.001f)
    {
        lastDrive = cachedDrive;
        repaint();
    }
}

juce::String OneKnobToRuleThemAllAudioProcessorEditor::getDriveStatusText (float drive)
{
    if (drive < 0.05f) return "The Shire (Bypass)";
    if (drive < 0.20f) return "Rivendell Warmth";
    if (drive < 0.35f) return "Rohan Drive";
    if (drive < 0.50f) return "Gondor Saturation";
    if (drive < 0.65f) return "Isengard Distortion";
    if (drive < 0.80f) return "Mount Doom Clipping";
    if (drive < 0.95f) return "Eye of Sauron";
    return "One Ring To Rule Them All";
}

//==============================================================================
void OneKnobToRuleThemAllAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Draw PNG Background if available
    if (bgImg.isValid())
    {
        g.drawImage (bgImg, getLocalBounds().toFloat());
    }
    else
    {
        // Fallback background
        juce::ColourGradient bg (juce::Colour (0xFF16171B), 0.0f, 0.0f, juce::Colour (0xFF0C0D0F), 0.0f, static_cast<float>(getHeight()), false);
        g.setGradientFill (bg);
        g.fillAll();
    }

    // ---- Title --------------------------------------------------------------
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold)); // LARGER TITLE
    g.drawText ("ONE KNOB", 0, 50, getWidth(), 30, juce::Justification::centred, true);
                
    g.setColour (juce::Colours::black); 
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold)); // LARGER SUBTITLE
    g.drawText ("TO RULE THEM ALL", 0, 80, getWidth(), 20, juce::Justification::centred, true);

    // ---- Dynamic Status Text ------------------------------------------------
    juce::Rectangle<int> statusArea (0, 410, getWidth(), 35);
    
    // Status label (e.g. "Heavy Distortion")
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold)); // LARGER STATUS
    g.drawText (getDriveStatusText(cachedDrive), statusArea, juce::Justification::centred, true);
    
    // Drive percentage
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold)); // LARGER PERCENTAGE
    g.drawText (juce::String(juce::roundToInt(cachedDrive * 100.0f)) + "%", 
                statusArea.translated(0, 24), juce::Justification::centred, true);

    // ---- Flavor Label -------------------------------------------------------
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("DWARVEN", 85,  460, 60, 20, juce::Justification::centred, false);
    g.drawText ("ELVEN",   150, 460, 60, 20, juce::Justification::centred, false);
    g.drawText ("MORDOR",  215, 460, 60, 20, juce::Justification::centred, false);
}

void OneKnobToRuleThemAllAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // ---- Drive knob (TRUE CENTER) -------------------------------------------
    const int knobSize = 300; // Increased size so the box doesn't cut the track
    driveSlider.setBounds (area.withSizeKeepingCentre (knobSize, knobSize));

    // ---- Flavor switch (SMALLER AND MOVED) ------------------------------
    modeSwitch.setBounds (105, 480, 150, 20); 
}
