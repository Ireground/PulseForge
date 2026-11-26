#pragma once
#include "PluginProcessor.h"
#include "Look.h"

class PulseForgeEditor;

//==============================================================================
// A button you drag onto a track (click alone just shows a hint)
class DragButton : public juce::TextButton
{
public:
    explicit DragButton (const juce::String& text) : juce::TextButton (text) {}
    void mouseDown (const juce::MouseEvent& e) override { started = false; juce::TextButton::mouseDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! started && e.getDistanceFromDragStart() > 4) { started = true; if (onDragStart) onDragStart(); }
    }
    std::function<void()> onDragStart;
private:
    bool started = false;
};

//==============================================================================
// Drum names + step grid
class StepGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit StepGrid (PulseForgeEditor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hoverRow = hoverStep = -1; repaint(); }
    void tick();
    float sc = 1.0f;

private:
    PulseForgeEditor& ed;
    PulseForgeProcessor& proc;
    int columns() const;
    juce::Rectangle<float> nameArea (int row) const;
    juce::Rectangle<float> muteArea (int row) const;
    juce::Rectangle<float> cellArea (int row, int step) const;
    int rowAt (float y) const;
    int stepAt (float x) const;
    float nameW() const { return 178.0f * sc; }
    float headerH() const { return 16.0f * sc; }
    float rowH() const { return (getHeight() - headerH()) / (float) PF::NumDrums; }

    int dragRow = -1, paintValue = -1, pressRow = -1, hoverRow = -1, hoverStep = -1;
    bool pressOnName = false, laneDragStarted = false, undoPushed = false;
    std::array<juce::uint32, PF::NumDrums> seenHits {};
    std::array<float, PF::NumDrums> flash {};
    int lastStep = -2, lastPattern = -1;
};

//==============================================================================
// Envelope picture of the selected drum
class EnvView : public juce::Component
{
public:
    explicit EnvView (PulseForgeProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    int drum = 0;
    float sc = 1.0f;
private:
    PulseForgeProcessor& proc;
};

//==============================================================================
class DrumPanel : public juce::Component
{
public:
    explicit DrumPanel (PulseForgeEditor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void setDrum (int d);
    void setScale (float s);
    void refresh() { env.repaint(); repaint (titleArea); }

private:
    PulseForgeEditor& ed;
    PulseForgeProcessor& proc;
    int drum = -1;
    float sc = 1.0f;
    Knob pitch, bend, btime, odecay, click, cutoff, reso, nattack, ndecay, stereo, mix, drive, level, pan, vel, mutateAmt;
    juce::ComboBox wave, fmode, nshape, outBox, chokeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAtt, fmodeAtt, nshapeAtt, outAtt, chokeAtt;
    juce::TextButton randomBtn { "RANDOM" }, mutateBtn { "MUTATE" }, resetBtn { "RESET" }, moreBtn { "MORE..." };
    EnvView env;
    juce::Rectangle<int> titleArea, toneArea, noiseArea, outArea;
};

//==============================================================================
class PulseForgeEditor : public juce::AudioProcessorEditor,
                         public juce::DragAndDropContainer,
                         private juce::Timer
{
public:
    explicit PulseForgeEditor (PulseForgeProcessor&);
    ~PulseForgeEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    PulseForgeProcessor& proc;
    void selectDrum (int d, bool play);
    void setStatus (const juce::String&);
    void hint (const juce::String&);
    void showLaneMenu (int drum);
    void renameDrum (int drum);
    void startMidiDrag (int onlyDrum);
    void refreshAll();

private:
    void timerCallback() override;
    void showKitMenu();
    void showMultiOutMenu();
    void showPatternMenu();
    void doMultiOut (bool onlyUsed);
    void updatePatternButtons();

    DeckLook look;
    StepGrid grid;
    DrumPanel panel;
    juce::TooltipWindow tooltips { this, 600 };

    juce::TextButton kitBtn { "KITS" }, randomKitBtn { "RANDOM KIT" }, undoBtn { "UNDO" }, redoBtn { "REDO" },
                     multiOutBtn { "MULTI OUT" }, multiMenuBtn { "..." }, patMenuBtn { "PATTERN..." },
                     playBtn { "PLAY" }, stepsModeBtn { "STEPS" }, probModeBtn { "CHANCE" };
    DragButton dragMidiBtn { "DRAG MIDI" };
    juce::ComboBox clipBox, lengthBox, rateBox;
    juce::ToggleButton seqToggle { "Sequencer" };
    std::array<juce::TextButton, PF::NumPatterns> patBtns;
    Knob master, swing;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqAtt;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> headerArea, toolbarArea, statusArea;
    float sc = 1.0f;
    bool sizeReady = false;
    juce::String status, hintText;
    juce::uint32 hintTime = 0, statusTime = 0;
    int shownPattern = -1, slowTick = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulseForgeEditor)
};
