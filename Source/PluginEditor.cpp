#include "PluginEditor.h"

using namespace juce;

namespace
{
    constexpr int baseW = 1120, baseH = 740;

    void fillChoices (ComboBox& box, AudioProcessorValueTreeState& s, const String& id)
    {
        box.clear (dontSendNotification);
        if (auto* c = dynamic_cast<AudioParameterChoice*> (s.getParameter (id)))
            for (int i = 0; i < c->choices.size(); ++i) box.addItem (c->choices[i].toUpperCase(), i + 1);
    }
}

//==============================================================================
// Step grid
StepGrid::StepGrid (PulseForgeEditor& e) : ed (e), proc (e.proc)
{
    for (int d = 0; d < PF::NumDrums; ++d) seenHits[(size_t) d] = proc.drumHits[(size_t) d].load();
}

int StepGrid::columns() const
{
    return proc.getPatternLength (proc.currentPattern.load()) <= 16 ? 16 : PF::MaxSteps;
}

Rectangle<float> StepGrid::nameArea (int row) const
{
    return { 0.0f, headerH() + row * rowH(), nameW() - 6.0f * sc, rowH() };
}

Rectangle<float> StepGrid::muteArea (int row) const
{
    auto n = nameArea (row).reduced (2.0f * sc);
    return n.removeFromRight (24.0f * sc).withSizeKeepingCentre (22.0f * sc, jmin (n.getHeight(), 20.0f * sc));
}

Rectangle<float> StepGrid::cellArea (int row, int step) const
{
    const float w = (getWidth() - nameW()) / (float) columns();
    return { nameW() + step * w, headerH() + row * rowH(), w, rowH() };
}

int StepGrid::rowAt (float y) const
{
    if (y < headerH()) return -1;
    const int r = (int) ((y - headerH()) / rowH());
    return isPositiveAndBelow (r, PF::NumDrums) ? r : -1;
}

int StepGrid::stepAt (float x) const
{
    if (x < nameW()) return -1;
    const int s = (int) ((x - nameW()) / ((getWidth() - nameW()) / (float) columns()));
    return isPositiveAndBelow (s, columns()) ? s : -1;
}

void StepGrid::paint (Graphics& g)
{
    const int pat = proc.currentPattern.load();
    const int len = proc.getPatternLength (pat);
    const int cols = columns();
    const int play = proc.displayStep.load();
    const bool probMode = proc.probMode;

    // step numbers
    g.setFont (uiFont (9.5f * sc, true));
    for (int s = 0; s < cols; ++s)
    {
        auto c = cellArea (0, s).withY (0).withHeight (headerH());
        g.setColour (s == play ? Pal::blue : (s % 4 == 0 ? Pal::dim : Pal::faint));
        if (s % 4 == 0 || s == play) g.drawText (String (s + 1), c, Justification::centred);
    }

    for (int d = 0; d < PF::NumDrums; ++d)
    {
        const auto col = PF::drumColour (d);
        const bool sel = d == proc.selectedDrum;
        const bool muted = proc.param (PF::pid (d, "mute")) > 0.5f;

        // name cell
        auto n = nameArea (d).reduced (0, 2.0f * sc);
        g.setColour (sel ? Pal::hover.brighter (0.15f) : (d == hoverRow && hoverStep < 0 ? Pal::hover : Pal::panel));
        g.fillRoundedRectangle (n, 6.0f * sc);
        if (sel) { g.setColour (col.withAlpha (0.8f)); g.drawRoundedRectangle (n.reduced (0.5f), 6.0f * sc, 1.2f); }
        if (flash[(size_t) d] > 0.01f) { g.setColour (col.withAlpha (0.35f * flash[(size_t) d])); g.fillRoundedRectangle (n, 6.0f * sc); }
        g.setColour (col);
        g.fillRoundedRectangle (n.withWidth (5.0f * sc).reduced (0, 4.0f * sc), 2.0f * sc);

        auto t = n.withTrimmedLeft (12.0f * sc).withTrimmedRight (30.0f * sc);
        const int out = proc.drumOut (d);
        auto pill = t.removeFromRight (40.0f * sc).withSizeKeepingCentre (38.0f * sc, jmin (t.getHeight() - 6.0f * sc, 16.0f * sc));
        drawPill (g, pill, "OUT " + String (out + 1), out == 0 ? Pal::badge : col.withAlpha (0.25f), out == 0 ? Pal::dim : col.brighter (0.4f), 9.0f * sc);
        g.setColour (muted ? Pal::dim : Pal::text);
        g.setFont (uiFont (12.5f * sc, true));
        g.drawFittedText (proc.getDrumName (d), t.toNearestInt(), Justification::centredLeft, 1, 0.8f);

        auto m = muteArea (d);
        g.setColour (muted ? Pal::red : Pal::badge);
        g.fillRoundedRectangle (m, 4.0f * sc);
        g.setColour (muted ? Colour (0xff0b1219) : Pal::dim);
        g.setFont (uiFont (10.5f * sc, true));
        g.drawText ("M", m, Justification::centred);

        // steps
        for (int s = 0; s < cols; ++s)
        {
            auto c = cellArea (d, s).reduced (1.5f * sc, 2.0f * sc);
            const bool active = s < len;
            const bool hov = d == hoverRow && s == hoverStep;
            g.setColour (! active ? Pal::panel.darker (0.2f) : ((s / 4) % 2 == 0 ? Pal::cellAlt : Pal::cell));
            g.fillRoundedRectangle (c, 3.5f * sc);
            if (hov && active) { g.setColour (Pal::blue.withAlpha (0.35f)); g.drawRoundedRectangle (c, 3.5f * sc, 1.0f); }

            const int lvl = proc.getStep (pat, d, s);
            if (lvl > 0 && active)
            {
                const int prob = proc.getProb (pat, d, s);
                auto f = c.reduced (2.0f * sc);
                auto filled = f.withTrimmedTop (f.getHeight() * (1.0f - prob / 100.0f));
                auto fc = lvl == 2 ? col.brighter (0.25f) : col.withAlpha (muted ? 0.3f : 0.72f);
                if (prob < 100) { g.setColour (fc.withAlpha (0.25f)); g.drawRoundedRectangle (f, 2.5f * sc, 1.0f); }
                g.setColour (fc);
                g.fillRoundedRectangle (filled, 2.5f * sc);
                if (lvl == 2) { g.setColour (Colours::white.withAlpha (0.85f)); g.drawRoundedRectangle (f, 2.5f * sc, 1.3f * sc); }
                if (probMode || (prob < 100 && c.getWidth() > 22.0f * sc))
                {
                    g.setColour (Colours::white.withAlpha (0.9f));
                    g.setFont (uiFont (8.5f * sc, true));
                    g.drawText (String (prob), c, Justification::centred);
                }
            }
            if (s == play && active) { g.setColour (Colours::white.withAlpha (0.14f)); g.fillRoundedRectangle (c, 3.5f * sc); }
        }
    }
}

void StepGrid::tick()
{
    bool dirty = false;
    for (int d = 0; d < PF::NumDrums; ++d)
    {
        const auto h = proc.drumHits[(size_t) d].load();
        if (h != seenHits[(size_t) d]) { seenHits[(size_t) d] = h; flash[(size_t) d] = 1.0f; dirty = true; }
        else if (flash[(size_t) d] > 0.0f) { flash[(size_t) d] = flash[(size_t) d] < 0.05f ? 0.0f : flash[(size_t) d] * 0.8f; dirty = true; }
    }
    const int st = proc.displayStep.load(), pat = proc.currentPattern.load();
    if (st != lastStep || pat != lastPattern) { lastStep = st; lastPattern = pat; dirty = true; }
    if (dirty) repaint();
}

void StepGrid::mouseMove (const MouseEvent& e)
{
    const int r = rowAt (e.position.y), s = stepAt (e.position.x);
    if (r != hoverRow || s != hoverStep) { hoverRow = r; hoverStep = s; repaint(); }
    if (r >= 0 && s < 0)
        setTooltip ("Click: play + edit this drum.  Drag onto a track: MIDI of this row.  Right-click: options");
    else if (r >= 0)
        setTooltip (proc.probMode ? "Click/drag up and down on a step to set its chance.  Right-click: back to 100%"
                                  : "Click: on > accent > off (drag to paint).  Right-click: clear");
    else setTooltip ({});
}

void StepGrid::mouseDown (const MouseEvent& e)
{
    const int r = rowAt (e.position.y);
    pressRow = r;
    pressOnName = false;
    laneDragStarted = false;
    undoPushed = false;
    paintValue = -1;
    dragRow = -1;
    if (r < 0) return;

    const int pat = proc.currentPattern.load();
    if (e.position.x < nameW())
    {
        if (muteArea (r).contains (e.position))
        {
            const auto id = PF::pid (r, "mute");
            proc.setParam (id, proc.param (id) > 0.5f ? 0.0f : 1.0f);
            repaint();
            return;
        }
        if (e.mods.isPopupMenu()) { ed.selectDrum (r, false); ed.showLaneMenu (r); return; }
        pressOnName = true;
        ed.selectDrum (r, true);
        return;
    }

    const int s = stepAt (e.position.x);
    if (s < 0 || s >= proc.getPatternLength (pat)) return;
    proc.pushUndo();
    undoPushed = true;
    dragRow = r;

    if (proc.probMode)
    {
        if (proc.getStep (pat, r, s) == 0) { proc.setStep (pat, r, s, 1); }
        if (e.mods.isPopupMenu()) { proc.setProb (pat, r, s, 100); paintValue = -2; }
        else mouseDrag (e);
    }
    else
    {
        paintValue = e.mods.isPopupMenu() ? 0 : (proc.getStep (pat, r, s) + 1) % 3;
        proc.setStep (pat, r, s, paintValue);
        if (paintValue == 0) proc.setProb (pat, r, s, 100);
    }
    repaint();
}

void StepGrid::mouseDrag (const MouseEvent& e)
{
    if (pressOnName)
    {
        if (! laneDragStarted && e.getDistanceFromDragStart() > 6)
        {
            laneDragStarted = true;
            ed.startMidiDrag (pressRow);
        }
        return;
    }
    if (dragRow < 0) return;
    const int pat = proc.currentPattern.load();
    const int r = rowAt (e.position.y), s = stepAt (e.position.x);
    if (s < 0 || s >= proc.getPatternLength (pat)) return;

    if (proc.probMode)
    {
        if (paintValue == -2) return;
        const int row = r >= 0 ? r : dragRow;
        auto c = cellArea (row, s);
        if (proc.getStep (pat, row, s) == 0) return;
        const float frac = 1.0f - jlimit (0.0f, 1.0f, (e.position.y - c.getY()) / c.getHeight());
        proc.setProb (pat, row, s, jmax (5, roundToInt (frac * 20.0f) * 5));
        ed.hint ("Chance: " + String (proc.getProb (pat, row, s)) + "%");
    }
    else if (r >= 0 && paintValue >= 0)
    {
        if (proc.getStep (pat, r, s) != paintValue)
        {
            proc.setStep (pat, r, s, paintValue);
            if (paintValue == 0) proc.setProb (pat, r, s, 100);
        }
    }
    repaint();
}

void StepGrid::mouseUp (const MouseEvent&)
{
    dragRow = -1;
    paintValue = -1;
    pressOnName = false;
}

//==============================================================================
// Envelope picture
void EnvView::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Pal::badge);
    g.fillRoundedRectangle (b, 8.0f * sc);
    g.setColour (Pal::panelEdge);
    g.drawRoundedRectangle (b.reduced (0.5f), 8.0f * sc, 1.0f);

    const auto s = proc.getSound (drum);
    const float mix = s[PF::Mix] * 0.01f;
    const float od = s[PF::OscDecay], na = s[PF::NoiseAttack], nd = s[PF::NoiseDecay], bt = s[PF::BendTime];
    const int shape = (int) s[PF::NoiseShape];
    float total = jmax (mix < 0.99f ? od : 0.0f, mix > 0.01f ? (shape == 3 ? 27.0f + nd : na + nd * (shape == 2 ? 1.15f : 1.0f)) : 0.0f, 40.0f);
    total = jmin (total * 1.05f, 4500.0f);

    auto plot = b.reduced (10.0f * sc, 8.0f * sc);
    plot.removeFromTop (14.0f * sc);
    plot.removeFromBottom (12.0f * sc);

    // grid
    g.setColour (Pal::faint.withAlpha (0.35f));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (plot.getY() + plot.getHeight() * i / 4.0f), plot.getX(), plot.getRight());

    auto curve = [&] (std::function<float (float)> f, Colour c, float alpha)
    {
        Path p;
        const int nPts = 120;
        for (int i = 0; i <= nPts; ++i)
        {
            const float tms = total * i / (float) nPts;
            const float y = plot.getBottom() - jlimit (0.0f, 1.0f, f (tms)) * plot.getHeight();
            const float x = plot.getX() + plot.getWidth() * i / (float) nPts;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (c.withAlpha (alpha));
        g.strokePath (p, PathStrokeType (1.8f * sc, PathStrokeType::curved, PathStrokeType::rounded));
    };

    auto expo = [] (float t, float decay) { return std::pow (0.001f, t / jmax (1.0f, decay)); };
    const float bend = s[PF::Bend];
    if (std::abs (bend) > 0.05f)
        curve ([&] (float t) { return 0.5f + 0.5f * (bend / 48.0f) * expo (t, bt); }, Pal::dim, 0.7f);
    if (mix < 0.99f)
        curve ([&] (float t) { return (1.0f - mix) * expo (t, od) + 0.0f; }, Pal::blue, 1.0f);
    if (mix > 0.01f)
        curve ([&] (float t)
               {
                   float e;
                   if (shape == 3)
                   {
                       if (t < 27.0f) e = std::exp (-std::fmod (t, 9.0f) / 2.5f);
                       else e = expo (t - 27.0f, nd);
                   }
                   else if (t < na) e = t / jmax (0.01f, na);
                   else
                   {
                       const float u = t - na;
                       e = shape == 1 ? jmax (0.0f, 1.0f - u / nd) : shape == 2 ? (u < nd ? 1.0f : expo (u - nd, 8.0f)) : expo (u, nd);
                   }
                   return e * mix;
               }, Pal::gold, 1.0f);

    g.setFont (uiFont (9.5f * sc, true));
    g.setColour (Pal::blue); g.drawText ("TONE", b.reduced (10.0f * sc, 6.0f * sc).removeFromTop (12.0f * sc), Justification::left);
    g.setColour (Pal::gold); g.drawText ("NOISE", b.reduced (10.0f * sc, 6.0f * sc).removeFromTop (12.0f * sc).withTrimmedLeft (40.0f * sc), Justification::left);
    g.setColour (Pal::dim); g.drawText ("PITCH", b.reduced (10.0f * sc, 6.0f * sc).removeFromTop (12.0f * sc).withTrimmedLeft (86.0f * sc), Justification::left);
    g.drawText (total < 1000.0f ? String (roundToInt (total)) + " ms" : String (total / 1000.0f, 2) + " s",
                b.reduced (10.0f * sc, 4.0f * sc).removeFromBottom (12.0f * sc), Justification::right);
}

//==============================================================================
// Drum panel
DrumPanel::DrumPanel (PulseForgeEditor& e)
    : ed (e), proc (e.proc),
      pitch (proc.apvts, "PITCH"), bend (proc.apvts, "BEND"), btime (proc.apvts, "BEND TIME"), odecay (proc.apvts, "DECAY"), click (proc.apvts, "CLICK"),
      cutoff (proc.apvts, "CUTOFF"), reso (proc.apvts, "RESO"), nattack (proc.apvts, "ATTACK"), ndecay (proc.apvts, "DECAY"), stereo (proc.apvts, "STEREO"),
      mix (proc.apvts, "TONE/NOISE"), drive (proc.apvts, "DRIVE"), level (proc.apvts, "LEVEL"), pan (proc.apvts, "PAN"), vel (proc.apvts, "VELOCITY"),
      mutateAmt (proc.apvts, "AMOUNT"), env (proc)
{
    for (auto* k : { &pitch, &bend, &btime, &odecay, &click, &cutoff, &reso, &nattack, &ndecay, &stereo, &mix, &drive, &level, &pan, &vel })
        addAndMakeVisible (k);
    for (auto* c : { &wave, &fmode, &nshape, &outBox, &chokeBox }) addAndMakeVisible (c);
    for (auto* b : { &randomBtn, &mutateBtn, &resetBtn, &moreBtn }) addAndMakeVisible (b);
    addAndMakeVisible (env);

    // the mutate amount is an editor setting, not a host parameter
    mutateAmt.slider.setRange (0.0, 100.0, 1.0);
    mutateAmt.slider.setValue (proc.mutateAmount * 100.0, dontSendNotification);
    mutateAmt.slider.setDoubleClickReturnValue (true, 35.0);
    mutateAmt.slider.textFromValueFunction = [] (double v) { return String (roundToInt (v)) + "%"; };
    mutateAmt.slider.onValueChange = [this] { proc.mutateAmount = (float) mutateAmt.slider.getValue() / 100.0f; };
    addAndMakeVisible (mutateAmt);

    setButtonStyle (randomBtn, Pal::gold, true);
    setButtonStyle (mutateBtn, Pal::blue, true);
    randomBtn.setTooltip ("New random sound that still suits this drum's role");
    mutateBtn.setTooltip ("Nudge the sound a little (AMOUNT sets how much)");
    resetBtn.setTooltip ("Back to this drum's sound in the Classic kit");
    moreBtn.setTooltip ("Rename, copy/paste, save/load this drum...");
    pitch.slider.setTooltip ("Pitch of the tone oscillator");
    bend.slider.setTooltip ("Pitch sweep at the start of the hit (the classic kick 'boom')");
    btime.slider.setTooltip ("How fast the pitch sweep settles");
    odecay.slider.setTooltip ("How long the tone rings");
    click.slider.setTooltip ("Short transient at the start, for punch");
    cutoff.slider.setTooltip ("Noise filter frequency");
    reso.slider.setTooltip ("Noise filter resonance");
    nattack.slider.setTooltip ("Fade-in of the noise (swells, reverse-ish hits)");
    ndecay.slider.setTooltip ("How long the noise lasts");
    stereo.slider.setTooltip ("Width of the noise (0 = mono)");
    mix.slider.setTooltip ("Balance between tone (left) and noise (right)");
    drive.slider.setTooltip ("Saturation");
    vel.slider.setTooltip ("How much MIDI velocity / accents change the level");
    wave.setTooltip ("Tone waveform");
    fmode.setTooltip ("Noise filter type");
    nshape.setTooltip ("Noise envelope: Exponential, Linear, Gated (holds, then stops) or Clap (three bursts + tail)");
    outBox.setTooltip ("Output this drum plays on. Several drums can share one");
    chokeBox.setTooltip ("Drums in the same group cut each other off (closed hat stops open hat)");

    randomBtn.onClick = [this] { proc.pushUndo(); proc.randomizeDrum (drum); proc.triggerDrum (drum); ed.refreshAll(); };
    mutateBtn.onClick = [this] { proc.pushUndo(); proc.mutateDrum (drum, proc.mutateAmount); proc.triggerDrum (drum); ed.refreshAll(); };
    resetBtn.onClick = [this] { proc.resetDrum (drum); proc.triggerDrum (drum); ed.refreshAll(); };
    moreBtn.onClick = [this] { ed.showLaneMenu (drum); };
}

void DrumPanel::setDrum (int d)
{
    if (d == drum) { refresh(); return; }
    drum = d;
    env.drum = d;
    pitch.attach (PF::pid (d, "pitch"));   bend.attach (PF::pid (d, "bend"));       btime.attach (PF::pid (d, "btime"));
    odecay.attach (PF::pid (d, "odecay")); click.attach (PF::pid (d, "click"));     cutoff.attach (PF::pid (d, "cutoff"));
    reso.attach (PF::pid (d, "reso"));     nattack.attach (PF::pid (d, "nattack")); ndecay.attach (PF::pid (d, "ndecay"));
    stereo.attach (PF::pid (d, "stereo")); mix.attach (PF::pid (d, "mix"));         drive.attach (PF::pid (d, "drive"));
    level.attach (PF::pid (d, "level"));   pan.attach (PF::pid (d, "pan"));         vel.attach (PF::pid (d, "velsens"));

    using CA = AudioProcessorValueTreeState::ComboBoxAttachment;
    waveAtt.reset(); fmodeAtt.reset(); nshapeAtt.reset(); outAtt.reset(); chokeAtt.reset();
    fillChoices (wave, proc.apvts, PF::pid (d, "wave"));
    fillChoices (fmode, proc.apvts, PF::pid (d, "fmode"));
    fillChoices (nshape, proc.apvts, PF::pid (d, "nshape"));
    fillChoices (outBox, proc.apvts, PF::pid (d, "out"));
    fillChoices (chokeBox, proc.apvts, PF::pid (d, "choke"));
    waveAtt = std::make_unique<CA> (proc.apvts, PF::pid (d, "wave"), wave);
    fmodeAtt = std::make_unique<CA> (proc.apvts, PF::pid (d, "fmode"), fmode);
    nshapeAtt = std::make_unique<CA> (proc.apvts, PF::pid (d, "nshape"), nshape);
    outAtt = std::make_unique<CA> (proc.apvts, PF::pid (d, "out"), outBox);
    chokeAtt = std::make_unique<CA> (proc.apvts, PF::pid (d, "choke"), chokeBox);
    refresh();
    repaint();
}

void DrumPanel::setScale (float s)
{
    sc = s;
    env.sc = s;
    for (auto* k : { &pitch, &bend, &btime, &odecay, &click, &cutoff, &reso, &nattack, &ndecay, &stereo, &mix, &drive, &level, &pan, &vel, &mutateAmt })
        k->uiScale = s;
}

void DrumPanel::resized()
{
    auto r = getLocalBounds().reduced ((int) (12 * sc), (int) (8 * sc));
    titleArea = r.removeFromTop ((int) (46 * sc));
    {
        auto t = titleArea;
        auto bw = [this] (int w) { return (int) (w * sc); };
        moreBtn.setBounds (t.removeFromRight (bw (84)).withSizeKeepingCentre (bw (84), bw (28)));
        t.removeFromRight (bw (8));
        resetBtn.setBounds (t.removeFromRight (bw (70)).withSizeKeepingCentre (bw (70), bw (28)));
        t.removeFromRight (bw (14));
        mutateAmt.setBounds (t.removeFromRight (bw (54)));
        mutateBtn.setBounds (t.removeFromRight (bw (90)).withSizeKeepingCentre (bw (90), bw (28)));
        t.removeFromRight (bw (8));
        randomBtn.setBounds (t.removeFromRight (bw (90)).withSizeKeepingCentre (bw (90), bw (28)));
    }
    r.removeFromTop ((int) (6 * sc));
    env.setBounds (r.removeFromRight ((int) (r.getWidth() * 0.22f)));
    r.removeFromRight ((int) (12 * sc));

    const int gap = (int) (12 * sc);
    const int secW = (r.getWidth() - 2 * gap) / 3;
    toneArea = r.removeFromLeft (secW); r.removeFromLeft (gap);
    noiseArea = r.removeFromLeft (secW); r.removeFromLeft (gap);
    outArea = r;

    auto layout = [this] (Rectangle<int> area, std::initializer_list<ComboBox*> combos, std::initializer_list<Knob*> knobs)
    {
        auto a = area.reduced ((int) (8 * sc), (int) (6 * sc));
        auto head = a.removeFromTop ((int) (26 * sc));
        head.removeFromLeft ((int) (58 * sc));   // section title
        const int cw = combos.size() > 0 ? head.getWidth() / (int) combos.size() : 0;
        for (auto* c : combos) c->setBounds (head.removeFromLeft (cw).reduced ((int) (2 * sc), 0));
        a.removeFromTop ((int) (6 * sc));
        const int kw = a.getWidth() / (int) knobs.size();
        for (auto* k : knobs) k->setBounds (a.removeFromLeft (kw).withSizeKeepingCentre (kw, jmin (a.getHeight(), (int) (kw * 1.45f))));
    };
    layout (toneArea, { &wave }, { &pitch, &bend, &btime, &odecay, &click });
    layout (noiseArea, { &fmode, &nshape }, { &cutoff, &reso, &nattack, &ndecay, &stereo });
    layout (outArea, { &outBox, &chokeBox }, { &mix, &drive, &level, &pan, &vel });
}

void DrumPanel::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Pal::panel);
    g.fillRoundedRectangle (b, 10.0f * sc);
    g.setColour (Pal::panelEdge);
    g.drawRoundedRectangle (b.reduced (0.5f), 10.0f * sc, 1.0f);
    if (drum < 0) return;

    const auto col = PF::drumColour (drum);
    auto t = titleArea.toFloat();
    auto badge = t.removeFromLeft (30.0f * sc).withSizeKeepingCentre (26.0f * sc, 26.0f * sc);
    g.setColour (col);
    g.fillRoundedRectangle (badge, 6.0f * sc);
    g.setColour (Colour (0xff0b1219));
    g.setFont (uiFont (13.0f * sc, true));
    g.drawText (String (drum + 1), badge, Justification::centred);
    t.removeFromLeft (10.0f * sc);
    g.setColour (Pal::text);
    g.setFont (uiFont (20.0f * sc, true));
    g.drawText (proc.getDrumName (drum), t.withWidth (260.0f * sc), Justification::centredLeft);
    g.setColour (Pal::dim);
    g.setFont (uiFont (10.0f * sc));
    g.drawText ("MIDI note " + MidiMessage::getMidiNoteName (PF::DrumNote + drum, true, true, 4) + "  -  double-click the name to rename",
                t.withTrimmedLeft (0).withTrimmedTop (30.0f * sc).withWidth (340.0f * sc), Justification::topLeft);

    auto section = [&] (Rectangle<int> a, const String& title)
    {
        auto f = a.toFloat();
        g.setColour (Pal::cell);
        g.fillRoundedRectangle (f, 8.0f * sc);
        g.setColour (Pal::panelEdge.withAlpha (0.7f));
        g.drawRoundedRectangle (f.reduced (0.5f), 8.0f * sc, 1.0f);
        g.setColour (col.withAlpha (0.9f));
        g.setFont (uiFont (11.0f * sc, true));
        g.drawText (title, f.reduced (10.0f * sc, 6.0f * sc).removeFromTop (26.0f * sc), Justification::centredLeft);
    };
    section (toneArea, "TONE");
    section (noiseArea, "NOISE");
    section (outArea, "OUTPUT");
}

void DrumPanel::mouseDown (const MouseEvent& e)
{
    auto name = titleArea.withTrimmedLeft ((int) (40 * sc)).withWidth ((int) (260 * sc));
    if (name.contains (e.getPosition()))
    {
        if (e.getNumberOfClicks() >= 2) ed.renameDrum (drum);
        else if (e.mods.isPopupMenu()) ed.showLaneMenu (drum);
        else proc.triggerDrum (drum);
    }
}

//==============================================================================
// Editor
PulseForgeEditor::PulseForgeEditor (PulseForgeProcessor& p)
    : AudioProcessorEditor (p), proc (p), grid (*this), panel (*this),
      master (p.apvts, "MASTER"), swing (p.apvts, "SWING")
{
    setLookAndFeel (&look);

    for (auto* c : std::initializer_list<Component*> { &grid, &panel, &kitBtn, &randomKitBtn, &undoBtn, &redoBtn, &multiOutBtn, &multiMenuBtn,
                                                       &patMenuBtn, &playBtn, &stepsModeBtn, &probModeBtn, &dragMidiBtn, &clipBox,
                                                       &lengthBox, &rateBox, &seqToggle, &master, &swing })
        addAndMakeVisible (c);

    for (int i = 0; i < PF::NumPatterns; ++i)
    {
        auto& b = patBtns[(size_t) i];
        b.setButtonText (String::charToString ((juce_wchar) ('A' + i)));
        b.setTooltip ("Pattern " + b.getButtonText() + " (MIDI note " + MidiMessage::getMidiNoteName (PF::PatternNote + i, true, true, 4)
                      + " switches to it live)");
        setButtonStyle (b, Pal::blue);
        b.onClick = [this, i]
        {
            proc.currentPattern = i;
            refreshAll();
        };
        addAndMakeVisible (b);
    }

    setButtonStyle (randomKitBtn, Pal::gold, true);
    setButtonStyle (multiOutBtn, Pal::blue, true);
    setButtonStyle (dragMidiBtn, Pal::blue, true);
    setButtonStyle (playBtn, Pal::green);
    setButtonStyle (stepsModeBtn, Pal::blue);
    setButtonStyle (probModeBtn, Pal::gold);

    kitBtn.setTooltip ("Factory kits, save / load your own kits");
    randomKitBtn.setTooltip ("New random sound on every drum (each stays a kick, snare, hat...)");
    undoBtn.setTooltip ("Undo the last kit or pattern change");
    redoBtn.setTooltip ("Redo");
    multiOutBtn.setTooltip ("Creates one REAPER track per output, with the routing done for you");
    multiMenuBtn.setTooltip ("Multi-out options");
    dragMidiBtn.setTooltip ("Drag this onto a track to get the current pattern as a MIDI clip");
    clipBox.setTooltip ("Length of the dragged MIDI clip (the pattern repeats to fill it)");
    patMenuBtn.setTooltip ("Clear, randomize, copy patterns, load a groove");
    playBtn.setTooltip ("Run the sequencer while REAPER is stopped (follows REAPER when it plays)");
    stepsModeBtn.setTooltip ("Edit steps: click = on > accent > off");
    probModeBtn.setTooltip ("Edit chance: drag up/down on a step to set how often it plays");
    lengthBox.setTooltip ("Pattern length");
    rateBox.setTooltip ("Step size");
    seqToggle.setTooltip ("Sequencer on/off (off = PulseForge only plays from MIDI)");
    master.slider.setTooltip ("Master volume");
    swing.slider.setTooltip ("Swing (delays every second step)");

    kitBtn.onClick = [this] { showKitMenu(); };
    randomKitBtn.onClick = [this] { proc.randomizeKit(); refreshAll(); setStatus ("New random kit - UNDO brings the old one back"); };
    undoBtn.onClick = [this] { proc.undo(); refreshAll(); };
    redoBtn.onClick = [this] { proc.redo(); refreshAll(); };
    multiOutBtn.onClick = [this] { doMultiOut (false); };
    multiMenuBtn.onClick = [this] { showMultiOutMenu(); };
    patMenuBtn.onClick = [this] { showPatternMenu(); };
    playBtn.setClickingTogglesState (true);
    playBtn.onClick = [this] { proc.internalPlay = playBtn.getToggleState(); };
    stepsModeBtn.onClick = [this] { proc.probMode = false; refreshAll(); };
    probModeBtn.onClick = [this] { proc.probMode = true; refreshAll(); };
    dragMidiBtn.onClick = [this] { setStatus ("Drag the DRAG MIDI button onto a track (don't just click it)"); };
    dragMidiBtn.onDragStart = [this] { startMidiDrag (-1); };

    clipBox.addItemList ({ "CLIP: PATTERN", "CLIP: 1 BAR", "CLIP: 2 BARS", "CLIP: 4 BARS", "CLIP: 8 BARS" }, 1);
    clipBox.setSelectedItemIndex (jlimit (0, 4, proc.clipLength), dontSendNotification);
    clipBox.onChange = [this] { proc.clipLength = jmax (0, clipBox.getSelectedItemIndex()); };

    for (int i = 1; i <= PF::MaxSteps; ++i) lengthBox.addItem (String (i) + " STEPS", i);
    lengthBox.onChange = [this]
    {
        const int n = lengthBox.getSelectedId();
        if (n > 0 && n != proc.getPatternLength (proc.currentPattern.load()))
        {
            proc.pushUndo();
            proc.setPatternLength (proc.currentPattern.load(), n);
            grid.repaint();
        }
    };

    fillChoices (rateBox, proc.apvts, "rate");
    rateAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "rate", rateBox);
    seqAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "seqon", seqToggle);
    master.attach ("master");
    swing.attach ("swing");

    // read the remembered size first: setResizeLimits() triggers resized(), which would overwrite it
    const int savedW = jlimit (880, 2240, proc.editorWidth), savedH = jlimit (580, 1480, proc.editorHeight);
    setResizable (true, true);
    setResizeLimits (880, 580, 2240, 1480);
    setSize (savedW, savedH);
    sizeReady = true;
    proc.editorWidth = getWidth();
    proc.editorHeight = getHeight();

    selectDrum (proc.selectedDrum, false);
    refreshAll();
    if (! proc.isInReaper()) status = "Tip: MULTI OUT builds the output tracks automatically in REAPER";
    startTimerHz (30);
}

PulseForgeEditor::~PulseForgeEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PulseForgeEditor::setStatus (const String& s)
{
    status = s;
    statusTime = Time::getMillisecondCounter();
    hintText.clear();
    repaint (statusArea);
}

void PulseForgeEditor::hint (const String& s)
{
    hintText = s;
    hintTime = Time::getMillisecondCounter();
    repaint (statusArea);
}

void PulseForgeEditor::selectDrum (int d, bool play)
{
    d = jlimit (0, PF::NumDrums - 1, d);
    proc.selectedDrum = d;
    panel.setDrum (d);
    if (play) proc.triggerDrum (d);
    grid.repaint();
}

void PulseForgeEditor::refreshAll()
{
    updatePatternButtons();
    stepsModeBtn.setToggleState (! proc.probMode, dontSendNotification);
    probModeBtn.setToggleState (proc.probMode, dontSendNotification);
    panel.setDrum (proc.selectedDrum);
    panel.refresh();
    grid.repaint();
}

void PulseForgeEditor::updatePatternButtons()
{
    const int cur = proc.currentPattern.load();
    for (int i = 0; i < PF::NumPatterns; ++i)
    {
        auto& b = patBtns[(size_t) i];
        b.setToggleState (i == cur, dontSendNotification);
        b.setAlpha (i == cur || ! proc.patternIsEmpty (i) ? 1.0f : 0.6f);
    }
    lengthBox.setSelectedId (proc.getPatternLength (cur), dontSendNotification);
    shownPattern = cur;
}

void PulseForgeEditor::timerCallback()
{
    grid.tick();
    if (proc.currentPattern.load() != shownPattern) { updatePatternButtons(); grid.repaint(); }
    undoBtn.setEnabled (proc.canUndo());
    redoBtn.setEnabled (proc.canRedo());
    playBtn.setToggleState (proc.internalPlay.load(), dontSendNotification);

    if (++slowTick % 6 == 0) panel.refresh();

    const auto now = Time::getMillisecondCounter();
    if (hintText.isNotEmpty() && now - hintTime > 2500) { hintText.clear(); repaint (statusArea); }
    if (status.isNotEmpty() && statusTime != 0 && now - statusTime > 9000) { status.clear(); statusTime = 0; repaint (statusArea); }
}

void PulseForgeEditor::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (Pal::bgTop, 0, 0, Pal::bgBottom, 0, (float) getHeight(), false));
    g.fillAll();

    // logo
    auto h = headerArea.toFloat().reduced (16 * sc, 0);
    const auto f = uiFont (23.0f * sc, true);
    const String a = "Pulse", b = "Forge";
    const float aw = GlyphArrangement::getStringWidth (f, a);
    g.setFont (f);
    g.setColour (Pal::text);
    g.drawText (a, h, Justification::centredLeft);
    g.setColour (Pal::blue);
    g.drawText (b, h.withTrimmedLeft (aw), Justification::centredLeft);

    // status bar
    auto s = statusArea.toFloat();
    g.setColour (Pal::badge);
    g.fillRect (s);
    g.setColour (Pal::panelEdge);
    g.drawLine (s.getX(), s.getY(), s.getRight(), s.getY(), 1.0f);
    auto sr = s.reduced (12 * sc, 0);
    g.setFont (uiFont (11.0f * sc, true));
    g.setColour (Pal::dim);
    g.drawText ("C1-G1 play the drums   C2-G2 switch patterns", sr, Justification::centredRight);
    g.setColour (hintText.isNotEmpty() || status.isNotEmpty() ? Pal::text : Pal::dim);
    g.setFont (uiFont (11.5f * sc, hintText.isNotEmpty()));
    const String left = hintText.isNotEmpty() ? hintText
                      : status.isNotEmpty() ? status
                      : String ("Click a drum name to hear and edit it  -  drag a drum name onto a track for that row's MIDI  -  right-click for options");
    g.drawFittedText (left, sr.withTrimmedRight ((int) (300 * sc)).toNearestInt(), Justification::centredLeft, 1, 0.8f);
}

void PulseForgeEditor::resized()
{
    if (sizeReady)
    {
        proc.editorWidth = getWidth();
        proc.editorHeight = getHeight();
    }
    sc = jmin ((float) getWidth() / (float) baseW, (float) getHeight() / (float) baseH);
    look.uiScale = sc;
    grid.sc = sc;
    panel.setScale (sc);
    master.uiScale = swing.uiScale = sc;
    auto bw = [this] (int v) { return (int) (v * sc); };

    auto r = getLocalBounds();
    headerArea = r.removeFromTop (bw (56));
    statusArea = r.removeFromBottom (bw (24));
    r = r.reduced (bw (12), bw (4));

    // header
    {
        auto hd = headerArea.reduced (bw (12), 0);
        hd.removeFromLeft (bw (150));
        auto btn = [&] (Component& c, int w, bool fromRight = false)
        {
            auto slot = fromRight ? hd.removeFromRight (bw (w)) : hd.removeFromLeft (bw (w));
            c.setBounds (slot.withSizeKeepingCentre (bw (w), bw (30)));
            if (fromRight) hd.removeFromRight (bw (8)); else hd.removeFromLeft (bw (8));
        };
        btn (kitBtn, 78); btn (randomKitBtn, 112); btn (undoBtn, 64); btn (redoBtn, 64);
        master.setBounds (hd.removeFromRight (bw (58)).withSizeKeepingCentre (bw (58), bw (54)));
        hd.removeFromRight (bw (10));
        btn (clipBox, 128, true);
        btn (dragMidiBtn, 112, true);
        btn (multiMenuBtn, 32, true);
        btn (multiOutBtn, 110, true);
    }

    // sequencer toolbar
    {
        auto tb = r.removeFromTop (bw (48));
        auto put = [&] (Component& c, int w, int h = 28)
        {
            c.setBounds (tb.removeFromLeft (bw (w)).withSizeKeepingCentre (bw (w), bw (h)));
            tb.removeFromLeft (bw (6));
        };
        for (auto& b : patBtns) put (b, 30);
        tb.removeFromLeft (bw (4));
        put (patMenuBtn, 98);
        put (lengthBox, 104);
        put (rateBox, 78);
        swing.setBounds (tb.removeFromLeft (bw (52)).withSizeKeepingCentre (bw (52), bw (48)));
        tb.removeFromLeft (bw (10));
        put (seqToggle, 112);
        auto right = tb;
        probModeBtn.setBounds (right.removeFromRight (bw (78)).withSizeKeepingCentre (bw (78), bw (28)));
        right.removeFromRight (bw (4));
        stepsModeBtn.setBounds (right.removeFromRight (bw (70)).withSizeKeepingCentre (bw (70), bw (28)));
        right.removeFromRight (bw (14));
        playBtn.setBounds (right.removeFromRight (bw (70)).withSizeKeepingCentre (bw (70), bw (28)));
    }

    r.removeFromTop (bw (4));
    grid.setBounds (r.removeFromTop (bw (262)));
    r.removeFromTop (bw (10));
    panel.setBounds (r.withTrimmedBottom (bw (6)));
}

//==============================================================================
// Menus & actions
void PulseForgeEditor::showKitMenu()
{
    PopupMenu m;
    m.addSectionHeader ("FACTORY KITS");
    const auto& kits = PF::factoryKits();
    for (int i = 0; i < (int) kits.size(); ++i) m.addItem (100 + i, kits[(size_t) i].name);
    m.addSeparator();
    m.addItem (1, "Save kit...");
    m.addItem (2, "Load kit...");
    m.addSeparator();
    m.addItem (3, "Give each drum its own output");
    m.addItem (4, "Put all drums on Out 1");
    m.addItem (5, "Unmute all drums");

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&kitBtn),
                     [this, safe = Component::SafePointer<PulseForgeEditor> (this)] (int r)
                     {
                         if (safe == nullptr || r == 0) return;
                         if (r >= 100) { proc.loadFactoryKit (r - 100); setStatus (String ("Loaded kit: ") + PF::factoryKits()[(size_t) (r - 100)].name); }
                         else if (r == 1)
                         {
                             chooser = std::make_unique<FileChooser> ("Save kit", PulseForgeProcessor::userFolder ("Kits").getChildFile ("My Kit.pfkit"), "*.pfkit");
                             chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                                                   [this, safe] (const FileChooser& fc)
                                                   {
                                                       if (safe == nullptr || fc.getResult() == File()) return;
                                                       const auto f = fc.getResult().withFileExtension ("pfkit");
                                                       setStatus (proc.saveKit (f) ? "Kit saved: " + f.getFileNameWithoutExtension() : String ("Could not save the kit"));
                                                   });
                         }
                         else if (r == 2)
                         {
                             chooser = std::make_unique<FileChooser> ("Load kit", PulseForgeProcessor::userFolder ("Kits"), "*.pfkit");
                             chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                                                   [this, safe] (const FileChooser& fc)
                                                   {
                                                       if (safe == nullptr || fc.getResult() == File()) return;
                                                       String err;
                                                       proc.loadKit (fc.getResult(), err);
                                                       setStatus (err.isNotEmpty() ? err : "Kit loaded: " + fc.getResult().getFileNameWithoutExtension());
                                                       refreshAll();
                                                   });
                         }
                         else if (r == 3) { for (int d = 0; d < PF::NumDrums; ++d) proc.setParam (PF::pid (d, "out"), (float) d); }
                         else if (r == 4) { for (int d = 0; d < PF::NumDrums; ++d) proc.setParam (PF::pid (d, "out"), 0.0f); }
                         else if (r == 5) { for (int d = 0; d < PF::NumDrums; ++d) proc.setParam (PF::pid (d, "mute"), 0.0f); }
                         refreshAll();
                     });
}

void PulseForgeEditor::showMultiOutMenu()
{
    PopupMenu m;
    m.addItem (1, "Create all 8 output tracks");
    m.addItem (2, "Create tracks only for outputs in use");
    m.addSeparator();
    m.addItem (3, "Give each drum its own output");
    m.addItem (4, "Put all drums on Out 1");
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&multiMenuBtn),
                     [this, safe = Component::SafePointer<PulseForgeEditor> (this)] (int r)
                     {
                         if (safe == nullptr || r == 0) return;
                         if (r == 1) doMultiOut (false);
                         else if (r == 2) doMultiOut (true);
                         else if (r == 3) for (int d = 0; d < PF::NumDrums; ++d) proc.setParam (PF::pid (d, "out"), (float) d);
                         else if (r == 4) for (int d = 0; d < PF::NumDrums; ++d) proc.setParam (PF::pid (d, "out"), 0.0f);
                         refreshAll();
                     });
}

void PulseForgeEditor::doMultiOut (bool onlyUsed)
{
    String msg;
    proc.createMultiOuts (onlyUsed, msg);
    setStatus (msg);
    refreshAll();
}

void PulseForgeEditor::showPatternMenu()
{
    const int cur = proc.currentPattern.load();
    const String name = String::charToString ((juce_wchar) ('A' + cur));
    PopupMenu m, copyTo, grooves;
    for (int i = 0; i < PF::NumPatterns; ++i)
        copyTo.addItem (200 + i, "Pattern " + String::charToString ((juce_wchar) ('A' + i)), i != cur);
    const auto names = PulseForgeProcessor::factoryPatternNames();
    for (int i = 0; i < names.size(); ++i) grooves.addItem (300 + i, names[i]);

    m.addSectionHeader ("PATTERN " + name);
    m.addItem (1, "Randomize pattern");
    m.addItem (2, "Clear pattern");
    m.addSubMenu ("Copy to", copyTo);
    m.addSubMenu ("Load a groove", grooves);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&patMenuBtn),
                     [this, cur, safe = Component::SafePointer<PulseForgeEditor> (this)] (int r)
                     {
                         if (safe == nullptr || r == 0) return;
                         proc.pushUndo();
                         if (r == 1) proc.randomizePattern (cur);
                         else if (r == 2) proc.clearPattern (cur);
                         else if (r >= 300) proc.loadFactoryPattern (cur, r - 300);
                         else if (r >= 200)
                         {
                             proc.copyPattern (cur, r - 200);
                             setStatus ("Copied to pattern " + String::charToString ((juce_wchar) ('A' + r - 200)));
                         }
                         refreshAll();
                     });
}

void PulseForgeEditor::showLaneMenu (int d)
{
    const int pat = proc.currentPattern.load();
    PopupMenu m;
    m.addSectionHeader (proc.getDrumName (d).toUpperCase());
    m.addItem (1, "Rename...");
    m.addSeparator();
    m.addItem (10, "Random steps");
    m.addItem (11, "Clear steps");
    m.addItem (12, "Shift steps left");
    m.addItem (13, "Shift steps right");
    m.addSeparator();
    m.addItem (20, "Random sound");
    m.addItem (21, "Mutate sound");
    m.addItem (22, "Reset sound");
    m.addItem (23, "Copy sound");
    m.addItem (24, "Paste sound", proc.hasCopiedDrum());
    m.addItem (25, "Save drum...");
    m.addItem (26, "Load drum...");
    m.addSeparator();
    m.addItem (30, "Tip: drag the drum name onto a track for this row's MIDI", false);

    m.showMenuAsync (PopupMenu::Options(),
                     [this, d, pat, safe = Component::SafePointer<PulseForgeEditor> (this)] (int r)
                     {
                         if (safe == nullptr || r == 0) return;
                         switch (r)
                         {
                             case 1: renameDrum (d); break;
                             case 10: proc.pushUndo(); proc.randomizeLane (pat, d); break;
                             case 11: proc.pushUndo(); proc.clearLane (pat, d); break;
                             case 12: proc.pushUndo(); proc.shiftLane (pat, d, -1); break;
                             case 13: proc.pushUndo(); proc.shiftLane (pat, d, 1); break;
                             case 20: proc.pushUndo(); proc.randomizeDrum (d); proc.triggerDrum (d); break;
                             case 21: proc.pushUndo(); proc.mutateDrum (d, proc.mutateAmount); proc.triggerDrum (d); break;
                             case 22: proc.resetDrum (d); proc.triggerDrum (d); break;
                             case 23: proc.copyDrum (d); setStatus ("Sound copied - right-click another drum to paste it"); break;
                             case 24: proc.pasteDrum (d); proc.triggerDrum (d); break;
                             case 25:
                             {
                                 const auto file = PulseForgeProcessor::userFolder ("Drums").getChildFile (File::createLegalFileName (proc.getDrumName (d)) + ".pfdrum");
                                 chooser = std::make_unique<FileChooser> ("Save drum", file, "*.pfdrum");
                                 chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                                                       [this, d, safe] (const FileChooser& fc)
                                                       {
                                                           if (safe == nullptr || fc.getResult() == File()) return;
                                                           const auto f = fc.getResult().withFileExtension ("pfdrum");
                                                           setStatus (proc.saveDrum (d, f) ? "Drum saved: " + f.getFileNameWithoutExtension() : String ("Could not save the drum"));
                                                       });
                                 break;
                             }
                             case 26:
                             {
                                 chooser = std::make_unique<FileChooser> ("Load drum", PulseForgeProcessor::userFolder ("Drums"), "*.pfdrum");
                                 chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                                                       [this, d, safe] (const FileChooser& fc)
                                                       {
                                                           if (safe == nullptr || fc.getResult() == File()) return;
                                                           String err;
                                                           proc.loadDrum (d, fc.getResult(), err);
                                                           if (err.isNotEmpty()) setStatus (err); else proc.triggerDrum (d);
                                                           refreshAll();
                                                       });
                                 break;
                             }
                             default: break;
                         }
                         refreshAll();
                     });
}

void PulseForgeEditor::renameDrum (int d)
{
    auto* w = new AlertWindow ("Rename drum", "New name for drum " + String (d + 1) + ":", MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.getDrumName (d));
    w->addButton ("OK", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
    w->enterModalState (true, ModalCallbackFunction::create ([this, d, w, safe = Component::SafePointer<PulseForgeEditor> (this)] (int r)
    {
        if (safe != nullptr && r == 1)
        {
            proc.pushUndo();
            proc.setDrumName (d, w->getTextEditorContents ("name"));
            refreshAll();
            panel.repaint();
        }
    }), true);
}

void PulseForgeEditor::startMidiDrag (int onlyDrum)
{
    const int pat = proc.currentPattern.load();
    if (onlyDrum >= 0 ? proc.laneIsEmpty (pat, onlyDrum) : proc.patternIsEmpty (pat))
    {
        setStatus (onlyDrum >= 0 ? "This row is empty - add some steps first" : "This pattern is empty - add some steps first");
        return;
    }
    const auto file = proc.writeMidiFile (onlyDrum);
    if (! file.existsAsFile()) { setStatus ("Could not write the MIDI file"); return; }

    Component::SafePointer<PulseForgeEditor> safe (this);
    DragAndDropContainer::performExternalDragDropOfFiles (StringArray { file.getFullPathName() }, false, this, [safe, onlyDrum]
    {
        if (safe == nullptr) return;
        if (onlyDrum >= 0)
        {
            safe->setStatus ("MIDI for " + safe->proc.getDrumName (onlyDrum) + " dragged (mute that row here if it now plays twice)");
            return;
        }
        // the clip now plays the drums: stop the sequencer doubling it
        if (safe->proc.param ("seqon") > 0.5f)
        {
            safe->proc.setParam ("seqon", 0.0f);
            safe->setStatus ("MIDI clip dragged. Sequencer switched off so hits don't play twice (turn it back on any time)");
        }
    });
}
