#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>
#include "ReaperBridge.h"

namespace PF
{
    constexpr int NumDrums = 8, MaxSteps = 32, NumPatterns = 8, NumOuts = 8;
    constexpr int DrumNote = 36;      // drum 1 = 36 ... drum 8 = 43
    constexpr int PatternNote = 48;   // pattern A = 48 ... pattern H = 55

    juce::String pid (int drum, const char* name);   // "d1_pitch" (drum is 0-based, id is 1-based)

    // The parameters that make up a drum's sound (saved in kits / drum presets)
    enum SoundParam { Wave, Pitch, Bend, BendTime, OscDecay, Click,
                      FilterMode, Cutoff, Reso, NoiseAttack, NoiseDecay, NoiseShape, Stereo,
                      Mix, Drive, Level, Pan, VelSens, Choke, NumSoundParams };
    const char* soundParamId (int sp);
    using Sound = std::array<float, NumSoundParams>;   // plain (real-world) values

    struct FactoryKit { const char* name; std::array<const char*, NumDrums> names; std::array<Sound, NumDrums> sounds; };
    const std::vector<FactoryKit>& factoryKits();

    juce::Colour drumColour (int drum);
    juce::StringArray rateNames();
    double rateBeats (int rateIndex);
}

class PulseForgeProcessor : public juce::AudioProcessor,
                            public juce::VST3ClientExtensions
{
public:
    PulseForgeProcessor();
    ~PulseForgeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PulseForge"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- REAPER ----
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return this; }
    void setIHostApplication (Steinberg::FUnknown* host) override { reaper.setHostContext (host); }
    bool isInReaper() const noexcept { return reaper.isReaper(); }
    bool createMultiOuts (bool onlyUsed, juce::String& message);

    // ---- parameters ----
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float param (const juce::String& id) const;
    void setParam (const juce::String& id, float plainValue);   // message thread, as a host gesture
    int drumOut (int drum) const { return (int) param (PF::pid (drum, "out")); }   // 0-based choice index

    // ---- sounds & kits (message thread) ----
    PF::Sound getSound (int drum) const;
    void setSound (int drum, const PF::Sound&);
    void loadFactoryKit (int index);
    void randomizeDrum (int drum);
    void randomizeKit();
    void mutateDrum (int drum, float amount);   // amount 0..1
    void resetDrum (int drum);                  // back to the drum's sound in the Classic kit
    void copyDrum (int drum);
    void pasteDrum (int drum);
    bool hasCopiedDrum() const { return clipboardValid; }
    bool saveKit (const juce::File&);
    bool loadKit (const juce::File&, juce::String& error);
    bool saveDrum (int drum, const juce::File&);
    bool loadDrum (int drum, const juce::File&, juce::String& error);
    static juce::File userFolder (const juce::String& sub);

    juce::String getDrumName (int drum) const;
    void setDrumName (int drum, const juce::String&);

    // undo for kit / pattern actions (not for single knob moves - the host undoes those)
    void pushUndo();
    void undo();
    void redo();
    bool canUndo() const { return ! undoStack.empty(); }
    bool canRedo() const { return ! redoStack.empty(); }

    // ---- auditioning ----
    void triggerDrum (int drum, int velocity = 110);   // any thread
    std::array<std::atomic<juce::uint32>, PF::NumDrums> drumHits {};   // counts up on every hit (row flash)

    // ---- sequencer (edited on the message thread, read by the audio thread) ----
    int getStep (int pattern, int drum, int step) const;   // 0 off, 1 on, 2 accent
    void setStep (int pattern, int drum, int step, int level);
    int getProb (int pattern, int drum, int step) const;   // 0..100 %
    void setProb (int pattern, int drum, int step, int percent);
    int getPatternLength (int pattern) const { return patLength[(size_t) juce::jlimit (0, PF::NumPatterns - 1, pattern)].load(); }
    void setPatternLength (int pattern, int steps);
    bool patternIsEmpty (int pattern) const;
    bool laneIsEmpty (int pattern, int drum) const;

    void clearPattern (int pattern);
    void randomizePattern (int pattern);
    void randomizeLane (int pattern, int drum);
    void clearLane (int pattern, int drum);
    void shiftLane (int pattern, int drum, int amount);
    void copyPattern (int from, int to);
    void loadFactoryPattern (int pattern, int which);
    static juce::StringArray factoryPatternNames();

    std::atomic<int> currentPattern { 0 };
    std::atomic<bool> internalPlay { false };   // PLAY button (runs the sequencer while the host is stopped)
    std::atomic<int> displayStep { -1 };

    // ---- MIDI export ----
    juce::File writeMidiFile (int onlyDrum = -1);   // -1 = all drums
    int clipLength = 0;                             // 0 pattern, 1/2/4/8 bars

    // ---- editor state ----
    int editorWidth = 1120, editorHeight = 740;
    int selectedDrum = 0;
    bool probMode = false;
    float mutateAmount = 0.35f;

private:
    static BusesProperties makeBuses();
    ReaperBridge reaper;
    juce::Random rng;   // message thread

    // names
    mutable juce::CriticalSection nameLock;
    std::array<juce::String, PF::NumDrums> names;

    // sequencer data
    static size_t stepIndex (int p, int d, int s) { return ((size_t) p * PF::NumDrums + (size_t) d) * PF::MaxSteps + (size_t) s; }
    std::array<std::atomic<juce::uint8>, (size_t) PF::NumPatterns * PF::NumDrums * PF::MaxSteps> steps {}, probs {};
    std::array<std::atomic<int>, PF::NumPatterns> patLength {};

    // undo snapshots
    std::unique_ptr<juce::XmlElement> makeSnapshot() const;
    void applySnapshot (const juce::XmlElement&);
    std::vector<std::unique_ptr<juce::XmlElement>> undoStack, redoStack;

    PF::Sound clipboard {};
    bool clipboardValid = false;
    juce::String clipboardName;

    // shared state helpers
    void writePatterns (juce::XmlElement&) const;
    void readPatterns (const juce::XmlElement&);
    void writeKit (juce::XmlElement&) const;
    bool readKit (const juce::XmlElement&);
    static void writeSound (juce::XmlElement&, const PF::Sound&);
    static PF::Sound readSound (const juce::XmlElement&, const PF::Sound& fallback);
    static PF::Sound clampSound (PF::Sound);

    // GUI -> audio triggers
    juce::AbstractFifo triggerFifo { 64 };
    std::array<std::pair<int, int>, 64> triggerData {};
    juce::CriticalSection triggerWriteLock;

    // ---- audio engine ----
    struct DrumParams
    {
        int wave = 0, filterMode = 0, noiseShape = 0, choke = 0, out = 0;
        double pitch = 50.0, bend = 0.0;
        float bendCoef = 0.99f, oscCoef = 0.99f, noiseCoef = 0.99f, noiseLinStep = 0.001f, noiseAttackStep = 1.0f;
        int noiseHoldSamples = 0;
        float click = 0.0f, stereo = 0.0f, mix = 0.0f, drive = 0.0f, driveGain = 1.0f, driveNorm = 1.0f;
        float g = 0.1f, k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;   // SVF
        float gainL = 1.0f, gainR = 1.0f, velSens = 0.5f;
        bool muted = false;
    };
    std::array<DrumParams, PF::NumDrums> dp;

    struct Voice
    {
        bool active = false;
        int drum = 0;
        double phase = 0.0, clickPhase = 0.0;
        float oscEnv = 0, pitchEnv = 0, noiseEnv = 0, clickEnv = 0, attack = 1.0f;
        int age = 0, noiseAge = 0;
        float vel = 1.0f;
        float fade = 1.0f, fadeStep = 0.0f;
        float gL = 0.0f, gR = 0.0f;           // smoothed gains
        float ic1[2] {}, ic2[2] {};           // filter state (L, R)
        juce::uint32 rnd = 1, rnd2 = 7;       // noise generators
    };
    std::array<Voice, PF::NumDrums * 2> voices;   // two per drum: the old one fades out on retrigger

    enum DrumParamIdx { RWave, RPitch, RBend, RBendTime, ROscDecay, RClick, RFilterMode, RCutoff, RReso, RNoiseAttack, RNoiseDecay,
                        RNoiseShape, RStereo, RMix, RDrive, RLevel, RPan, RVelSens, RChoke, ROut, RMute, NumRaw };
    std::array<std::array<std::atomic<float>*, NumRaw>, PF::NumDrums> raw {};
    std::atomic<float>* rawMaster = nullptr; std::atomic<float>* rawSwing = nullptr;
    std::atomic<float>* rawSeqOn = nullptr; std::atomic<float>* rawRate = nullptr;
    float rp (int d, int which) const noexcept { return raw[(size_t) d][(size_t) which]->load(); }
    void updateDrumParams();
    void startVoice (int drum, float velocity01);
    void renderVoices (int start, int num);
    void resetVoices();

    struct Event { int offset; int drum; float vel; };
    std::array<Event, 512> events;
    int numEvents = 0;
    void addEvent (int offset, int drum, float vel);
    void sequence (int numSamples);
    juce::uint32 audioRnd = 0x1234567;
    float audioRandom01() noexcept;

    std::array<float*, PF::NumOuts * 2> busPtr {};
    double sr = 44100.0;
    double freePpq = 0.0;
    bool wasRunning = false;
    double lastEndPpq = -1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulseForgeProcessor)
};
