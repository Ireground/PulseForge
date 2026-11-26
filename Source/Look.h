#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

// StageDeck colour scheme (FabFilter Pro-R style dark blue)
namespace Pal
{
    const juce::Colour bgTop { 0xff22303f }, bgBottom { 0xff111a23 }, panel { 0xff0e161f }, panelEdge { 0xff2a3a4b },
        text { 0xffe9eef3 }, dim { 0xff8394a6 }, faint { 0xff3b4d60 }, blue { 0xff5ab4e8 }, gold { 0xfff2c14e },
        knobTop { 0xff3c4c5e }, knobBottom { 0xff17212b }, badge { 0xff0d151d },
        cell { 0xff16212c }, cellAlt { 0xff1b2835 }, hover { 0xff1a2633 }, red { 0xffe0606a }, green { 0xff6fcf8f };
}

juce::Font uiFont (float h, bool bold = false);

//==============================================================================
class DeckLook : public juce::LookAndFeel_V4
{
public:
    DeckLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                        int thumbStart, int thumbSize, bool over, bool down) override;
    int getDefaultScrollbarWidth() override { return juce::jmax (8, (int) (10 * uiScale)); }
    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    float uiScale = 1.0f;
};

// TextButton properties understood by DeckLook:
//   "accent" (int ARGB)  - colour when toggled on, or always when "solid"
//   "solid"  (bool)      - big filled action button
void setButtonStyle (juce::Button&, juce::Colour accent, bool solid = false);

//==============================================================================
// Knob drag with a real fine mode: hold Shift for 10x finer moves. Pressing or
// releasing Shift mid-drag carries on from where the knob is (no jump).
class FineSlider : public juce::Slider
{
public:
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    float pixelsForFullRange = 180.0f;
private:
    juce::Point<float> lastPos;
    double dragProportion = 0.0;
    bool ownDrag = false;
};

//==============================================================================
// Rotary knob bound to a parameter; the parameter can be swapped (selected pad changes)
class Knob : public juce::Component, private juce::Slider::Listener
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& title);
    ~Knob() override;
    void attach (const juce::String& paramId);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent&) override { repaint(); }

    FineSlider slider;
    float uiScale = 1.0f;
private:
    void sliderValueChanged (juce::Slider*) override { repaint(); }
    juce::AudioProcessorValueTreeState& state;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::String title, paramId;
};

void drawPill (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour bg, juce::Colour fg, float fontH);
