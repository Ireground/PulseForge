#include "Look.h"

using namespace juce;

Font uiFont (float h, bool bold)
{
    auto o = FontOptions (h);
    return Font (bold ? o.withStyle ("Bold") : o);
}

void setButtonStyle (Button& b, Colour accent, bool solid)
{
    b.getProperties().set ("accent", (int) accent.getARGB());
    b.getProperties().set ("solid", solid);
}

void drawPill (Graphics& g, Rectangle<float> r, const String& text, Colour bg, Colour fg, float fontH)
{
    g.setColour (bg);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (fg);
    g.setFont (uiFont (fontH, true));
    g.drawText (text, r, Justification::centred);
}

//==============================================================================
DeckLook::DeckLook()
{
    setColour (PopupMenu::backgroundColourId, Pal::panel);
    setColour (PopupMenu::textColourId, Pal::text);
    setColour (PopupMenu::headerTextColourId, Pal::dim);
    setColour (PopupMenu::highlightedBackgroundColourId, Pal::blue.withAlpha (0.35f));
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (ComboBox::textColourId, Pal::text);
    setColour (ComboBox::backgroundColourId, Pal::panel);
    setColour (ComboBox::outlineColourId, Pal::panelEdge);
    setColour (ComboBox::arrowColourId, Pal::dim);
    setColour (TextButton::textColourOffId, Pal::text);
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (ToggleButton::textColourId, Pal::dim);
    setColour (TextEditor::textColourId, Pal::text);
    setColour (TextEditor::backgroundColourId, Pal::panel);
    setColour (TextEditor::highlightColourId, Pal::blue.withAlpha (0.35f));
    setColour (TextEditor::outlineColourId, Pal::panelEdge);
    setColour (TextEditor::focusedOutlineColourId, Pal::blue);
    setColour (CaretComponent::caretColourId, Pal::blue);
    setColour (ScrollBar::thumbColourId, Pal::faint);
    setColour (Label::textColourId, Pal::text);
}

void DeckLook::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f * 0.74f;
    const auto c = b.getCentre();
    const float angle = start + pos * (end - start);
    const bool bipolar = (bool) s.getProperties()["bipolar"];

    g.setColour (Pal::faint);
    for (int i = 0; i < 11; ++i)
    {
        const float a = start + (float) i / 10.0f * (end - start);
        const auto p = c.getPointOnCircumference (r * 1.22f, a);
        g.fillEllipse (p.x - 1.1f * uiScale, p.y - 1.1f * uiScale, 2.2f * uiScale, 2.2f * uiScale);
    }

    {
        Path arc;
        const float from = bipolar ? (start + end) * 0.5f : start;
        arc.addCentredArc (c.x, c.y, r * 1.08f, r * 1.08f, 0.0f, jmin (from, angle), jmax (from, angle), true);
        g.setColour (Pal::blue.withAlpha (s.isMouseOverOrDragging() ? 1.0f : 0.75f));
        g.strokePath (arc, PathStrokeType (2.0f * uiScale, PathStrokeType::curved, PathStrokeType::rounded));
    }

    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - r, c.y - r + 2.0f * uiScale, r * 2, r * 2);
    g.setGradientFill (ColourGradient (Pal::knobTop, c.x, c.y - r, Pal::knobBottom, c.x, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawEllipse (c.x - r + 0.5f, c.y - r + 0.5f, r * 2 - 1, r * 2 - 1, 1.0f);

    const auto p1 = c.getPointOnCircumference (r * 0.3f, angle);
    const auto p2 = c.getPointOnCircumference (r * 0.95f, angle);
    g.setColour (Colours::white);
    g.drawLine ({ p1, p2 }, 2.2f * uiScale);
}

void DeckLook::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const float rad = jmin (r.getHeight() * 0.5f, 7.0f * uiScale);
    const auto& props = b.getProperties();
    const Colour accent = props.contains ("accent") ? Colour ((uint32) (int) props["accent"]) : Pal::blue;
    const bool solid = (bool) props["solid"];
    const bool on = b.getToggleState();
    const bool enabled = b.isEnabled();

    if (solid)
    {
        auto c = accent.withMultipliedSaturation (enabled ? 1.0f : 0.2f).withMultipliedBrightness (enabled ? 1.0f : 0.5f);
        if (over && enabled) c = c.brighter (0.12f);
        if (down) c = c.darker (0.15f);
        g.setGradientFill (ColourGradient (c.brighter (0.08f), 0, r.getY(), c.darker (0.12f), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, rad);
        g.setColour (Colours::white.withAlpha (0.15f));
        g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
        return;
    }

    if (on)
    {
        g.setColour (accent.withAlpha (down ? 0.6f : (over ? 0.85f : 0.75f)));
        g.fillRoundedRectangle (r, rad);
        return;
    }
    g.setColour (down ? accent.withAlpha (0.25f) : (over ? Pal::hover : Pal::panel));
    g.fillRoundedRectangle (r, rad);
    g.setColour (over ? accent.withAlpha (0.7f) : Pal::panelEdge);
    g.drawRoundedRectangle (r, rad, 1.0f);
}

Font DeckLook::getTextButtonFont (TextButton&, int h)
{
    return uiFont (jmin (13.0f * uiScale, (float) h * 0.55f), true);
}

void DeckLook::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    const bool solid = (bool) b.getProperties()["solid"];
    const bool on = b.getToggleState();
    g.setColour (! b.isEnabled() ? Pal::faint : (solid ? Colour (0xff0b1219) : (on ? Colour (0xff0b1219) : Pal::text)));
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced ((int) (6 * uiScale), 0), Justification::centred, 1, 0.8f);
}

void DeckLook::drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const float h = 14.0f * uiScale, w = 26.0f * uiScale;
    auto sw = Rectangle<float> (r.getX(), r.getCentreY() - h * 0.5f, w, h);
    const bool on = b.getToggleState();
    g.setColour (on ? Pal::blue.withAlpha (0.85f) : Pal::faint);
    g.fillRoundedRectangle (sw, h * 0.5f);
    const float d = h - 4.0f;
    g.setColour (Colours::white);
    g.fillEllipse (on ? sw.getRight() - d - 2.0f : sw.getX() + 2.0f, sw.getY() + 2.0f, d, d);
    g.setColour (over ? Pal::text : Pal::dim);
    g.setFont (uiFont (11.5f * uiScale, true));
    g.drawText (b.getButtonText().toUpperCase(), r.withTrimmedLeft (w + 6.0f * uiScale), Justification::centredLeft);
}

void DeckLook::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    const float rad = jmin (r.getHeight() * 0.5f, 7.0f * uiScale);
    g.setColour (box.isMouseOver (true) ? Pal::hover : Pal::panel);
    g.fillRoundedRectangle (r, rad);
    g.setColour (box.isMouseOver (true) ? Pal::blue.withAlpha (0.7f) : Pal::panelEdge);
    g.drawRoundedRectangle (r, rad, 1.0f);
    Path arrow;
    const float ax = (float) w - 11.0f * uiScale, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 3.5f * uiScale, ay - 1.8f * uiScale, ax + 3.5f * uiScale, ay - 1.8f * uiScale, ax, ay + 2.6f * uiScale);
    g.setColour (Pal::dim);
    g.fillPath (arrow);
}

Font DeckLook::getComboBoxFont (ComboBox& b) { return uiFont (jmin (12.5f * uiScale, b.getHeight() * 0.6f), true); }
Font DeckLook::getPopupMenuFont() { return uiFont (14.0f * uiScale); }

void DeckLook::positionComboBoxText (ComboBox& box, Label& l)
{
    l.setBounds ((int) (6 * uiScale), 0, box.getWidth() - (int) (18 * uiScale), box.getHeight());
    l.setFont (getComboBoxFont (box));
    l.setJustificationType (Justification::centred);
}

void DeckLook::drawScrollbar (Graphics& g, ScrollBar&, int x, int y, int w, int h, bool vertical,
                              int thumbStart, int thumbSize, bool over, bool down)
{
    auto t = vertical ? Rectangle<int> (x, thumbStart, w, thumbSize) : Rectangle<int> (thumbStart, y, thumbSize, h);
    g.setColour ((over || down) ? Pal::dim.withAlpha (0.8f) : Pal::faint);
    g.fillRoundedRectangle (t.toFloat().reduced (2.0f), 3.0f);
}

void DeckLook::fillTextEditorBackground (Graphics& g, int w, int h, TextEditor&)
{
    g.setColour (Pal::panel);
    g.fillRoundedRectangle (Rectangle<float> (0, 0, (float) w, (float) h), jmin (h * 0.5f, 7.0f * uiScale));
}

void DeckLook::drawTextEditorOutline (Graphics& g, int w, int h, TextEditor& t)
{
    g.setColour (t.hasKeyboardFocus (true) ? Pal::blue : Pal::panelEdge);
    g.drawRoundedRectangle (Rectangle<float> (0.5f, 0.5f, (float) w - 1, (float) h - 1), jmin (h * 0.5f, 7.0f * uiScale), 1.0f);
}

//==============================================================================
void FineSlider::mouseDown (const MouseEvent& e)
{
    Slider::mouseDown (e);   // starts the host gesture, handles alt-click reset and right-click
    ownDrag = isEnabled() && ! e.mods.isPopupMenu() && ! e.mods.isAltDown();
    lastPos = e.position;
    dragProportion = valueToProportionOfLength (getValue());
}

void FineSlider::mouseDrag (const MouseEvent& e)
{
    if (! ownDrag) { Slider::mouseDrag (e); return; }
    const float delta = (e.position.x - lastPos.x) + (lastPos.y - e.position.y);
    lastPos = e.position;
    const double speed = e.mods.isShiftDown() ? 0.1 : 1.0;
    dragProportion = jlimit (0.0, 1.0, dragProportion + delta * speed / jmax (20.0f, pixelsForFullRange));
    setValue (snapValue (proportionOfLengthToValue (dragProportion), absoluteDrag), sendNotificationSync);
}

void FineSlider::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    if (! e.mods.isShiftDown()) { Slider::mouseWheelMove (e, w); return; }
    auto fine = w;
    fine.deltaX *= 0.1f;
    fine.deltaY *= 0.1f;
    Slider::mouseWheelMove (e, fine);
}

//==============================================================================
Knob::Knob (AudioProcessorValueTreeState& s, const String& t) : state (s), title (t)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    slider.setMouseDragSensitivity (180);
    addAndMakeVisible (slider);
    slider.addListener (this);
    slider.addMouseListener (this, false);
}

Knob::~Knob()
{
    attachment.reset();
    slider.removeListener (this);
}

void Knob::attach (const String& id)
{
    if (id == paramId) return;
    paramId = id;
    attachment.reset();
    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (state, id, slider);
    if (auto* param = state.getParameter (id))
    {
        const float def = param->convertFrom0to1 (param->getDefaultValue());
        slider.setDoubleClickReturnValue (true, def);
        const auto range = param->getNormalisableRange();
        slider.getProperties().set ("bipolar", range.start < 0.0f && range.end > 0.0f && std::abs (range.start + range.end) < 1.0e-3f);
    }
    repaint();
}

void Knob::resized()
{
    auto b = getLocalBounds();
    b.removeFromBottom (jmax (12, (int) (15 * uiScale)));
    slider.setBounds (b);
}

void Knob::paint (Graphics& g)
{
    const int labelH = jmax (12, (int) (15 * uiScale));
    auto lab = getLocalBounds().removeFromBottom (labelH).toFloat();
    const bool active = slider.isMouseOverOrDragging();
    g.setFont (uiFont (10.5f * uiScale, true));
    g.setColour (active ? Pal::blue : Pal::dim);
    g.drawText (active ? slider.getTextFromValue (slider.getValue()) : title, lab, Justification::centred);
}
