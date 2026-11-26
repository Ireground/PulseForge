#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>
#include <iterator>

using namespace juce;

//==============================================================================
namespace PF
{
    String pid (int drum, const char* name) { return "d" + String (drum + 1) + "_" + name; }

    const char* soundParamId (int sp)
    {
        static const char* ids[] = { "wave", "pitch", "bend", "btime", "odecay", "click",
                                     "fmode", "cutoff", "reso", "nattack", "ndecay", "nshape", "stereo",
                                     "mix", "drive", "level", "pan", "velsens", "choke" };
        return ids[jlimit (0, NumSoundParams - 1, sp)];
    }

    Colour drumColour (int d)
    {
        static const uint32 c[] = { 0xffe8834a, 0xfff2c14e, 0xffe0606a, 0xff5ab4e8, 0xff4fd1c5, 0xffa98bf0, 0xff6fcf8f, 0xfff07ac0 };
        return Colour (c[jlimit (0, NumDrums - 1, d)]);
    }

    StringArray rateNames() { return { "1/8", "1/16", "1/32", "1/8 T", "1/16 T", "1/32 T" }; }

    double rateBeats (int i)
    {
        static const double b[] = { 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0, 1.0 / 12.0 };
        return b[jlimit (0, 5, i)];
    }

    // wave, pitch, bend, bendTime, oscDecay, click, filter, cutoff, reso, nAttack, nDecay, nShape, stereo, mix, drive, level, pan, velSens, choke
    const std::vector<FactoryKit>& factoryKits()
    {
        static const std::vector<FactoryKit> kits {
            { "Classic", { "Kick", "Snare", "Clap", "CH Hat", "OH Hat", "Tom", "Rim", "Zap" }, {{
                { 0, 52, 36, 45, 450, 15, 0, 1500, 10, 0, 8, 0, 0, 4, 25, 0, 0, 50, 0 },
                { 1, 185, 7, 30, 120, 10, 1, 3000, 15, 0, 180, 0, 30, 60, 15, -1, 0, 50, 0 },
                { 0, 220, 0, 10, 10, 0, 1, 1200, 30, 0, 220, 3, 50, 100, 30, -2, 10, 50, 0 },
                { 3, 5400, 0, 10, 40, 0, 2, 7000, 10, 0, 45, 0, 40, 85, 10, -5, 25, 50, 1 },
                { 3, 5400, 0, 10, 250, 0, 2, 6500, 10, 0, 400, 0, 40, 85, 10, -6, 25, 50, 1 },
                { 0, 90, 10, 120, 350, 10, 0, 800, 10, 0, 40, 0, 0, 10, 10, -1.5f, -30, 50, 0 },
                { 1, 800, 12, 8, 40, 30, 1, 2500, 50, 0, 20, 0, 0, 30, 20, -4, -20, 50, 0 },
                { 0, 400, 36, 25, 120, 0, 0, 2000, 10, 0, 10, 0, 0, 0, 10, -6, 30, 50, 0 } }} },
            { "EDM Punch", { "Kick", "Snare", "Clap", "Hat", "Open Hat", "Perc", "Shaker", "Laser" }, {{
                { 0, 48, 40, 30, 380, 35, 0, 3000, 10, 0, 6, 0, 0, 3, 45, 0, 0, 40, 0 },
                { 1, 200, 12, 20, 90, 20, 1, 4200, 20, 0, 200, 1, 50, 70, 25, -2, 0, 40, 0 },
                { 0, 300, 0, 10, 10, 0, 1, 1500, 25, 0, 300, 3, 70, 100, 35, -1, 0, 40, 0 },
                { 3, 6200, 0, 10, 30, 0, 2, 8500, 15, 0, 35, 0, 50, 90, 15, -6, 20, 60, 1 },
                { 3, 6200, 0, 10, 200, 0, 2, 8000, 15, 2, 320, 1, 50, 90, 15, -7, 20, 50, 1 },
                { 0, 420, 20, 15, 90, 15, 1, 3000, 40, 0, 25, 0, 0, 15, 30, -6, -25, 50, 0 },
                { 0, 1000, 0, 10, 5, 0, 2, 6000, 20, 25, 60, 1, 80, 100, 0, -10, -15, 60, 0 },
                { 2, 900, 30, 80, 200, 0, 0, 3000, 40, 0, 10, 0, 0, 0, 30, -10, 30, 50, 0 } }} },
            { "Techno Warehouse", { "Kick", "Snare", "Clap", "Hat", "Open Hat", "Tom", "Rim", "Noise" }, {{
                { 0, 45, 30, 55, 700, 20, 0, 800, 20, 0, 30, 0, 0, 6, 60, -1, 0, 30, 0 },
                { 3, 160, 5, 30, 80, 10, 1, 1800, 40, 0, 150, 0, 20, 70, 40, -4, 0, 40, 0 },
                { 0, 300, 0, 10, 10, 0, 1, 1000, 40, 0, 400, 3, 60, 100, 40, -4, -10, 40, 0 },
                { 3, 4800, 0, 10, 50, 0, 2, 9000, 30, 0, 50, 2, 30, 80, 25, -8, 15, 60, 1 },
                { 3, 4800, 0, 10, 300, 0, 2, 7500, 30, 5, 600, 0, 40, 80, 25, -9, 15, 50, 1 },
                { 1, 110, 8, 150, 300, 5, 0, 600, 10, 0, 30, 0, 0, 5, 30, -3, -35, 50, 0 },
                { 3, 1700, 0, 10, 25, 40, 1, 3500, 60, 0, 15, 0, 0, 25, 30, -8, 35, 50, 0 },
                { 0, 200, 0, 10, 5, 0, 1, 900, 70, 150, 1500, 1, 90, 100, 20, -14, 0, 30, 0 } }} },
            { "808 Style", { "Kick", "Snare", "Clap", "Hat", "Open Hat", "Low Tom", "Cowbell", "Clave" }, {{
                { 0, 50, 14, 40, 1500, 5, 0, 500, 0, 0, 5, 0, 0, 0, 15, 0, 0, 50, 0 },
                { 0, 230, 4, 15, 140, 5, 2, 1800, 10, 0, 170, 0, 20, 55, 5, -2, 0, 50, 0 },
                { 0, 300, 0, 10, 10, 0, 1, 1100, 20, 0, 260, 3, 40, 100, 5, -3, 0, 50, 0 },
                { 3, 7000, 0, 10, 30, 0, 2, 8000, 10, 0, 40, 0, 20, 90, 5, -7, 10, 60, 1 },
                { 3, 7000, 0, 10, 300, 0, 2, 7500, 10, 0, 450, 0, 20, 90, 5, -8, 10, 50, 1 },
                { 0, 85, 6, 80, 450, 3, 0, 500, 5, 0, 20, 0, 0, 5, 5, -2, -25, 50, 0 },
                { 3, 560, 0, 10, 160, 5, 1, 800, 20, 0, 10, 0, 0, 0, 20, -9, 20, 50, 0 },
                { 1, 2500, 0, 10, 35, 10, 1, 2500, 50, 0, 10, 0, 0, 0, 0, -7, -20, 50, 0 } }} },
            { "Glitch Lab", { "Thump", "Crack", "Fizz", "Tick", "Spray", "Bloop", "Bit", "Sweep" }, {{
                { 3, 60, 48, 15, 200, 50, 0, 400, 30, 0, 10, 0, 0, 5, 80, -3, 0, 40, 0 },
                { 2, 300, 24, 5, 50, 40, 2, 2500, 30, 0, 90, 2, 80, 70, 60, -6, 0, 40, 0 },
                { 0, 100, 0, 10, 5, 0, 1, 5000, 85, 0, 180, 3, 100, 100, 50, -8, 0, 40, 0 },
                { 3, 9000, 0, 10, 8, 60, 2, 12000, 20, 0, 6, 0, 0, 50, 40, -9, 40, 60, 1 },
                { 0, 100, 0, 10, 5, 0, 2, 3000, 60, 60, 250, 1, 100, 100, 30, -12, -40, 60, 1 },
                { 0, 700, -30, 60, 150, 0, 0, 2000, 10, 0, 10, 0, 0, 0, 20, -8, -30, 50, 0 },
                { 3, 1300, 48, 3, 30, 0, 1, 3000, 40, 0, 10, 0, 0, 0, 90, -12, 30, 50, 0 },
                { 2, 150, 36, 400, 600, 0, 0, 2000, 30, 0, 10, 0, 0, 20, 40, -12, 0, 30, 0 } }} },
        };
        return kits;
    }
}

namespace
{
    constexpr float outputCeiling = 7.94f;   // +18 dBFS: anything above is a runaway, never music

    float dbToGain (float db) { return db <= -47.9f ? 0.0f : Decibels::decibelsToGain (db); }

    inline float fastTanh (float x) noexcept
    {
        x = jlimit (-3.0f, 3.0f, x);
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    inline float polyBlep (double t, double dt) noexcept
    {
        if (t < dt) { t /= dt; return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }

    inline float noise (uint32& s) noexcept
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) (int32) s * (1.0f / 2147483648.0f);
    }

    String pct (float v, int) { return String (roundToInt (v)) + "%"; }
    String hz (float v, int) { return v < 1000.0f ? String (roundToInt (v)) + " Hz" : String (v / 1000.0f, v < 10000.0f ? 2 : 1) + " kHz"; }
    String ms (float v, int) { return v < 1000.0f ? String (v, v < 10.0f ? 1 : 0) + " ms" : String (v / 1000.0f, 2) + " s"; }
    String db (float v, int) { return v <= -47.9f ? String ("-inf dB") : String (v, 1) + " dB"; }
    String st (float v, int) { return (v > 0 ? "+" : "") + String (v, 1) + " st"; }
    String panText (float v, int) { return std::abs (v) < 0.5f ? String ("C") : (v < 0 ? "L" : "R") + String (roundToInt (std::abs (v))); }

    NormalisableRange<float> logRange (float lo, float hi, float centre)
    {
        NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    // factory patterns: '.' off, 'x' on, 'X' accent, '?' on with 50% chance
    struct FactoryPattern { const char* name; const char* rows[PF::NumDrums]; };
    const FactoryPattern factoryPatterns[] = {
        { "Four on the floor", { "X...x...x...x...", "....x.......x...", "....X.......X...", "xx.xxx.xxx.xxx.x",
                                 "..x...x...x...x.", "..............x.", ".......x...x...x", "................" } },
        { "Breakbeat",         { "X.....x...x.....", "....X.......X..?", "................", "x?x.x?x.x?x.x?x.",
                                 "..............x.", "..........?..x..", "...x......x.....", "................" } },
        { "Techno roll",       { "X...x...x...x...", "................", "....x.......x...", "x.?.x.?.x.?.x.??",
                                 "..x...x...x...x.", "......x.....x..?", "?..?..?..?..?...", ".?.............." } },
        { "Half-time",         { "X.......x.?.....", "........X.......", "........x.......", "x.x.x.x.x.x.x.xx",
                                 "................", "..............x.", "................", "...............?" } },
    };
}

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout PulseForgeProcessor::createLayout()
{
    using F = AudioParameterFloat;
    using C = AudioParameterChoice;
    using B = AudioParameterBool;
    using A = AudioParameterFloatAttributes;
    AudioProcessorValueTreeState::ParameterLayout l;
    const auto& k = PF::factoryKits().front().sounds;

    l.add (std::make_unique<F> (ParameterID { "master", 1 }, "Master", NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f,
                                A().withStringFromValueFunction (db)));
    l.add (std::make_unique<F> (ParameterID { "swing", 1 }, "Swing", NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f,
                                A().withStringFromValueFunction (pct)));
    l.add (std::make_unique<B> (ParameterID { "seqon", 1 }, "Sequencer", true));
    l.add (std::make_unique<C> (ParameterID { "rate", 1 }, "Step Rate", PF::rateNames(), 1));

    for (int d = 0; d < PF::NumDrums; ++d)
    {
        const String n = "Drum " + String (d + 1) + " ";
        const auto& s = k[(size_t) d];
        auto id = [d] (const char* x) { return ParameterID { PF::pid (d, x), 1 }; };
        l.add (std::make_unique<C> (id ("wave"), n + "Wave", StringArray { "Sine", "Triangle", "Saw", "Square" }, (int) s[PF::Wave]));
        l.add (std::make_unique<F> (id ("pitch"), n + "Pitch", logRange (20.0f, 12000.0f, 300.0f), s[PF::Pitch], A().withStringFromValueFunction (hz)));
        l.add (std::make_unique<F> (id ("bend"), n + "Bend", NormalisableRange<float> (-48.0f, 48.0f, 0.1f), s[PF::Bend], A().withStringFromValueFunction (st)));
        l.add (std::make_unique<F> (id ("btime"), n + "Bend Time", logRange (1.0f, 2000.0f, 60.0f), s[PF::BendTime], A().withStringFromValueFunction (ms)));
        l.add (std::make_unique<F> (id ("odecay"), n + "Tone Decay", logRange (5.0f, 4000.0f, 300.0f), s[PF::OscDecay], A().withStringFromValueFunction (ms)));
        l.add (std::make_unique<F> (id ("click"), n + "Click", NormalisableRange<float> (0.0f, 100.0f, 1.0f), s[PF::Click], A().withStringFromValueFunction (pct)));
        l.add (std::make_unique<C> (id ("fmode"), n + "Noise Filter", StringArray { "Low Pass", "Band Pass", "High Pass" }, (int) s[PF::FilterMode]));
        l.add (std::make_unique<F> (id ("cutoff"), n + "Cutoff", logRange (30.0f, 20000.0f, 1500.0f), s[PF::Cutoff], A().withStringFromValueFunction (hz)));
        l.add (std::make_unique<F> (id ("reso"), n + "Resonance", NormalisableRange<float> (0.0f, 100.0f, 1.0f), s[PF::Reso], A().withStringFromValueFunction (pct)));
        l.add (std::make_unique<F> (id ("nattack"), n + "Noise Attack", logRange (0.0f, 300.0f, 25.0f), s[PF::NoiseAttack], A().withStringFromValueFunction (ms)));
        l.add (std::make_unique<F> (id ("ndecay"), n + "Noise Decay", logRange (5.0f, 4000.0f, 300.0f), s[PF::NoiseDecay], A().withStringFromValueFunction (ms)));
        l.add (std::make_unique<C> (id ("nshape"), n + "Noise Envelope", StringArray { "Exponential", "Linear", "Gated", "Clap" }, (int) s[PF::NoiseShape]));
        l.add (std::make_unique<F> (id ("stereo"), n + "Noise Stereo", NormalisableRange<float> (0.0f, 100.0f, 1.0f), s[PF::Stereo], A().withStringFromValueFunction (pct)));
        l.add (std::make_unique<F> (id ("mix"), n + "Tone/Noise Mix", NormalisableRange<float> (0.0f, 100.0f, 1.0f), s[PF::Mix], A().withStringFromValueFunction (pct)));
        l.add (std::make_unique<F> (id ("drive"), n + "Drive", NormalisableRange<float> (0.0f, 100.0f, 1.0f), s[PF::Drive], A().withStringFromValueFunction (pct)));
        l.add (std::make_unique<F> (id ("level"), n + "Level", NormalisableRange<float> (-48.0f, 6.0f, 0.1f), s[PF::Level], A().withStringFromValueFunction (db)));
        l.add (std::make_unique<F> (id ("pan"), n + "Pan", NormalisableRange<float> (-100.0f, 100.0f, 1.0f), s[PF::Pan], A().withStringFromValueFunction (panText)));
        l.add (std::make_unique<F> (id ("velsens"), n + "Velocity", NormalisableRange<float> (0.0f, 100.0f, 1.0f), s[PF::VelSens], A().withStringFromValueFunction (pct)));
        l.add (std::make_unique<C> (id ("choke"), n + "Choke Group", StringArray { "Off", "Group 1", "Group 2", "Group 3" }, (int) s[PF::Choke]));
        StringArray outs;
        for (int o = 1; o <= PF::NumOuts; ++o) outs.add ("Out " + String (o));
        l.add (std::make_unique<C> (id ("out"), n + "Output", outs, 0));
        l.add (std::make_unique<B> (id ("mute"), n + "Mute", false));
    }
    return l;
}

AudioProcessor::BusesProperties PulseForgeProcessor::makeBuses()
{
    auto b = AudioProcessor::BusesProperties().withOutput ("Out 1", AudioChannelSet::stereo(), true);
    for (int i = 2; i <= PF::NumOuts; ++i)
        b = b.withOutput ("Out " + String (i), AudioChannelSet::stereo(), true);
    return b;
}

PulseForgeProcessor::PulseForgeProcessor()
    : AudioProcessor (makeBuses()),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    static const char* rawNames[] = { "wave", "pitch", "bend", "btime", "odecay", "click", "fmode", "cutoff", "reso", "nattack", "ndecay",
                                      "nshape", "stereo", "mix", "drive", "level", "pan", "velsens", "choke", "out", "mute" };
    for (int d = 0; d < PF::NumDrums; ++d)
        for (int i = 0; i < NumRaw; ++i)
            raw[(size_t) d][(size_t) i] = apvts.getRawParameterValue (PF::pid (d, rawNames[i]));
    rawMaster = apvts.getRawParameterValue ("master");
    rawSwing = apvts.getRawParameterValue ("swing");
    rawSeqOn = apvts.getRawParameterValue ("seqon");
    rawRate = apvts.getRawParameterValue ("rate");

    const auto& kit = PF::factoryKits().front();
    for (int d = 0; d < PF::NumDrums; ++d) names[(size_t) d] = kit.names[(size_t) d];
    for (auto& p : patLength) p = 16;
    loadFactoryPattern (0, 0);
    loadFactoryPattern (1, 1);
    loadFactoryPattern (2, 2);
    loadFactoryPattern (3, 3);
    for (int i = 0; i < (int) voices.size(); ++i) { voices[(size_t) i].rnd = 0x9e3779b9u * (uint32) (i + 1); voices[(size_t) i].rnd2 = 0x85ebca6bu * (uint32) (i + 3); }
}

float PulseForgeProcessor::param (const String& id) const
{
    if (auto* v = apvts.getRawParameterValue (id)) return v->load();
    return 0.0f;
}

void PulseForgeProcessor::setParam (const String& id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        const float v = p->convertTo0to1 (plainValue);
        if (std::abs (p->getValue() - v) < 1.0e-7f) return;
        p->beginChangeGesture();
        p->setValueNotifyingHost (v);
        p->endChangeGesture();
    }
}

bool PulseForgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::stereo()) return false;
    for (int i = 1; i < layouts.outputBuses.size(); ++i)
    {
        const auto& s = layouts.outputBuses.getReference (i);
        if (! s.isDisabled() && s != AudioChannelSet::stereo()) return false;
    }
    return layouts.inputBuses.isEmpty() || layouts.getMainInputChannelSet().isDisabled();
}

void PulseForgeProcessor::prepareToPlay (double sampleRate, int)
{
    sr = (sampleRate > 1000.0 && sampleRate < 1.0e6) ? sampleRate : 44100.0;
    resetVoices();
    numEvents = 0;
    wasRunning = false;
}

void PulseForgeProcessor::resetVoices()
{
    for (auto& v : voices)
    {
        v.active = false;
        v.ic1[0] = v.ic1[1] = v.ic2[0] = v.ic2[1] = 0.0f;
    }
}

//==============================================================================
// Sounds & kits
PF::Sound PulseForgeProcessor::getSound (int d) const
{
    PF::Sound s {};
    for (int i = 0; i < PF::NumSoundParams; ++i) s[(size_t) i] = param (PF::pid (d, PF::soundParamId (i)));
    return s;
}

void PulseForgeProcessor::setSound (int d, const PF::Sound& s)
{
    const auto c = clampSound (s);
    for (int i = 0; i < PF::NumSoundParams; ++i) setParam (PF::pid (d, PF::soundParamId (i)), c[(size_t) i]);
}

PF::Sound PulseForgeProcessor::clampSound (PF::Sound s)
{
    static const float lo[] = { 0, 20, -48, 1, 5, 0, 0, 30, 0, 0, 5, 0, 0, 0, 0, -48, -100, 0, 0 };
    static const float hi[] = { 3, 12000, 48, 2000, 4000, 100, 2, 20000, 100, 300, 4000, 3, 100, 100, 100, 6, 100, 100, 3 };
    for (int i = 0; i < PF::NumSoundParams; ++i)
    {
        float v = s[(size_t) i];
        if (! std::isfinite (v)) v = lo[i];
        s[(size_t) i] = jlimit (lo[i], hi[i], v);
    }
    return s;
}

void PulseForgeProcessor::loadFactoryKit (int index)
{
    const auto& kits = PF::factoryKits();
    if (! isPositiveAndBelow (index, (int) kits.size())) return;
    pushUndo();
    for (int d = 0; d < PF::NumDrums; ++d)
    {
        setSound (d, kits[(size_t) index].sounds[(size_t) d]);
        setDrumName (d, kits[(size_t) index].names[(size_t) d]);
    }
}

void PulseForgeProcessor::randomizeDrum (int d)
{
    if (! isPositiveAndBelow (d, PF::NumDrums)) return;
    auto r = [this] (float a, float b) { return a + rng.nextFloat() * (b - a); };
    auto lr = [this] (float a, float b) { return a * std::pow (b / a, rng.nextFloat()); };
    auto pick = [this] (std::initializer_list<int> opts) { return (float) *(opts.begin() + rng.nextInt ((int) opts.size())); };

    PF::Sound s = getSound (d);
    s[PF::VelSens] = r (30, 60);
    s[PF::NoiseAttack] = 0;
    s[PF::NoiseShape] = 0;
    s[PF::Stereo] = 0;
    s[PF::Click] = 0;
    s[PF::Pan] = r (-15, 15);
    switch (d)
    {
        case 0:   // kick
            s[PF::Wave] = rng.nextFloat() < 0.8f ? 0.0f : 1.0f; s[PF::Pitch] = r (40, 65); s[PF::Bend] = r (15, 48); s[PF::BendTime] = lr (15, 80);
            s[PF::OscDecay] = lr (200, 900); s[PF::Click] = r (0, 50); s[PF::FilterMode] = 0; s[PF::Cutoff] = lr (300, 3000); s[PF::Reso] = r (0, 30);
            s[PF::NoiseDecay] = lr (5, 40); s[PF::Mix] = r (0, 10); s[PF::Drive] = r (0, 70); s[PF::Level] = r (-2, 0); s[PF::Pan] = 0; break;
        case 1:   // snare
            s[PF::Wave] = pick ({ 0, 1, 3 }); s[PF::Pitch] = lr (150, 300); s[PF::Bend] = r (0, 24); s[PF::BendTime] = lr (5, 60);
            s[PF::OscDecay] = lr (50, 200); s[PF::Click] = r (0, 30); s[PF::FilterMode] = pick ({ 1, 2 }); s[PF::Cutoff] = lr (1500, 6000);
            s[PF::Reso] = r (0, 50); s[PF::NoiseAttack] = r (0, 3); s[PF::NoiseDecay] = lr (100, 350); s[PF::NoiseShape] = pick ({ 0, 1, 2 });
            s[PF::Stereo] = r (0, 60); s[PF::Mix] = r (40, 85); s[PF::Drive] = r (0, 50); s[PF::Level] = r (-4, -1); break;
        case 2:   // clap
            s[PF::Wave] = 0; s[PF::Pitch] = lr (200, 400); s[PF::Bend] = 0; s[PF::BendTime] = 10; s[PF::OscDecay] = lr (5, 20);
            s[PF::FilterMode] = 1; s[PF::Cutoff] = lr (800, 2500); s[PF::Reso] = r (10, 50); s[PF::NoiseDecay] = lr (150, 500);
            s[PF::NoiseShape] = rng.nextFloat() < 0.8f ? 3.0f : 2.0f; s[PF::Stereo] = r (30, 80); s[PF::Mix] = r (90, 100);
            s[PF::Drive] = r (0, 50); s[PF::Level] = r (-4, -1); break;
        case 3:   // closed hat
        case 4:   // open hat
            s[PF::Wave] = rng.nextFloat() < 0.7f ? 3.0f : 2.0f; s[PF::Pitch] = lr (3000, 9000); s[PF::Bend] = 0; s[PF::BendTime] = 10;
            s[PF::OscDecay] = d == 3 ? lr (15, 60) : lr (100, 400); s[PF::FilterMode] = 2; s[PF::Cutoff] = lr (6000, 12000); s[PF::Reso] = r (0, 40);
            s[PF::NoiseAttack] = d == 4 ? r (0, 5) : 0.0f; s[PF::NoiseDecay] = d == 3 ? lr (25, 80) : lr (200, 700);
            s[PF::NoiseShape] = pick ({ 0, 0, 1, 2 }); s[PF::Stereo] = r (20, 70); s[PF::Mix] = r (60, 100); s[PF::Drive] = r (0, 40);
            s[PF::Level] = r (-9, -4); s[PF::Pan] = r (-30, 30); break;
        case 5:   // tom
            s[PF::Wave] = pick ({ 0, 0, 1 }); s[PF::Pitch] = lr (70, 220); s[PF::Bend] = r (3, 24); s[PF::BendTime] = lr (40, 200);
            s[PF::OscDecay] = lr (150, 500); s[PF::Click] = r (0, 20); s[PF::FilterMode] = 0; s[PF::Cutoff] = lr (400, 2000); s[PF::Reso] = r (0, 20);
            s[PF::NoiseDecay] = lr (10, 60); s[PF::Mix] = r (0, 20); s[PF::Drive] = r (0, 40); s[PF::Level] = r (-4, -1); s[PF::Pan] = r (-40, 40); break;
        case 6:   // rim / perc
            s[PF::Wave] = pick ({ 0, 1, 2, 3 }); s[PF::Pitch] = lr (400, 2500); s[PF::Bend] = r (0, 24); s[PF::BendTime] = lr (3, 30);
            s[PF::OscDecay] = lr (15, 80); s[PF::Click] = r (10, 60); s[PF::FilterMode] = 1; s[PF::Cutoff] = lr (1500, 5000); s[PF::Reso] = r (30, 80);
            s[PF::NoiseDecay] = lr (10, 40); s[PF::Mix] = r (10, 50); s[PF::Drive] = r (0, 50); s[PF::Level] = r (-8, -3); s[PF::Pan] = r (-40, 40); break;
        default:  // fx
            s[PF::Wave] = pick ({ 0, 1, 2, 3 }); s[PF::Pitch] = lr (100, 2000); s[PF::Bend] = r (-48, 48); s[PF::BendTime] = lr (10, 500);
            s[PF::OscDecay] = lr (50, 600); s[PF::FilterMode] = pick ({ 0, 1, 2 }); s[PF::Cutoff] = lr (500, 8000); s[PF::Reso] = r (0, 70);
            s[PF::NoiseAttack] = rng.nextFloat() < 0.3f ? r (0, 100) : 0.0f; s[PF::NoiseDecay] = lr (10, 800); s[PF::NoiseShape] = pick ({ 0, 1, 2 });
            s[PF::Stereo] = r (0, 80); s[PF::Mix] = r (0, 50); s[PF::Drive] = r (0, 70); s[PF::Level] = r (-12, -6); s[PF::Pan] = r (-40, 40); break;
    }
    setSound (d, s);
}

void PulseForgeProcessor::randomizeKit()
{
    pushUndo();
    for (int d = 0; d < PF::NumDrums; ++d) randomizeDrum (d);
}

void PulseForgeProcessor::mutateDrum (int d, float amount)
{
    if (! isPositiveAndBelow (d, PF::NumDrums)) return;
    amount = jlimit (0.0f, 1.0f, amount);
    PF::Sound s = getSound (d);
    for (int i = 0; i < PF::NumSoundParams; ++i)
    {
        auto* p = apvts.getParameter (PF::pid (d, PF::soundParamId (i)));
        if (p == nullptr || i == PF::Choke) continue;
        if (i == PF::Level) { s[(size_t) i] += (rng.nextFloat() * 2.0f - 1.0f) * amount * 3.0f; continue; }
        if (dynamic_cast<AudioParameterChoice*> (p) != nullptr)
        {
            if (i != PF::FilterMode && rng.nextFloat() < amount * 0.25f)
                s[(size_t) i] = (float) rng.nextInt (p->getNumSteps());
            continue;
        }
        float n = p->convertTo0to1 (s[(size_t) i]);
        n = jlimit (0.0f, 1.0f, n + (rng.nextFloat() * 2.0f - 1.0f) * amount * 0.25f);
        s[(size_t) i] = p->convertFrom0to1 (n);
    }
    setSound (d, s);
}

void PulseForgeProcessor::resetDrum (int d)
{
    if (! isPositiveAndBelow (d, PF::NumDrums)) return;
    pushUndo();
    setSound (d, PF::factoryKits().front().sounds[(size_t) d]);
}

void PulseForgeProcessor::copyDrum (int d)
{
    clipboard = getSound (d);
    clipboardName = getDrumName (d);
    clipboardValid = true;
}

void PulseForgeProcessor::pasteDrum (int d)
{
    if (! clipboardValid) return;
    pushUndo();
    auto s = clipboard;
    s[PF::Choke] = getSound (d)[PF::Choke];   // keep the slot's choke group
    setSound (d, s);
    setDrumName (d, clipboardName);
}

String PulseForgeProcessor::getDrumName (int d) const
{
    const ScopedLock sl (nameLock);
    return isPositiveAndBelow (d, PF::NumDrums) ? names[(size_t) d] : String();
}

void PulseForgeProcessor::setDrumName (int d, const String& n)
{
    if (! isPositiveAndBelow (d, PF::NumDrums)) return;
    const ScopedLock sl (nameLock);
    names[(size_t) d] = n.trim().substring (0, 24);
    if (names[(size_t) d].isEmpty()) names[(size_t) d] = "Drum " + String (d + 1);
}

File PulseForgeProcessor::userFolder (const String& sub)
{
    auto f = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("PulseForge").getChildFile (sub);
    f.createDirectory();
    return f;
}

void PulseForgeProcessor::writeSound (XmlElement& e, const PF::Sound& s)
{
    for (int i = 0; i < PF::NumSoundParams; ++i) e.setAttribute (PF::soundParamId (i), s[(size_t) i]);
}

PF::Sound PulseForgeProcessor::readSound (const XmlElement& e, const PF::Sound& fallback)
{
    PF::Sound s = fallback;
    for (int i = 0; i < PF::NumSoundParams; ++i)
        s[(size_t) i] = (float) e.getDoubleAttribute (PF::soundParamId (i), fallback[(size_t) i]);
    return clampSound (s);
}

void PulseForgeProcessor::writeKit (XmlElement& root) const
{
    for (int d = 0; d < PF::NumDrums; ++d)
    {
        auto* e = root.createNewChildElement ("DRUM");
        e->setAttribute ("index", d);
        e->setAttribute ("name", getDrumName (d));
        writeSound (*e, getSound (d));
    }
}

bool PulseForgeProcessor::readKit (const XmlElement& root)
{
    bool any = false;
    for (auto* e : root.getChildWithTagNameIterator ("DRUM"))
    {
        const int d = e->getIntAttribute ("index", -1);
        if (! isPositiveAndBelow (d, PF::NumDrums)) continue;
        setSound (d, readSound (*e, getSound (d)));
        setDrumName (d, e->getStringAttribute ("name", getDrumName (d)));
        any = true;
    }
    return any;
}

bool PulseForgeProcessor::saveKit (const File& f)
{
    XmlElement root ("PULSEFORGE_KIT");
    root.setAttribute ("version", 1);
    writeKit (root);
    return root.writeTo (f);
}

bool PulseForgeProcessor::loadKit (const File& f, String& error)
{
    auto xml = XmlDocument::parse (f);
    if (xml == nullptr || ! xml->hasTagName ("PULSEFORGE_KIT")) { error = "That isn't a PulseForge kit file"; return false; }
    pushUndo();
    if (! readKit (*xml)) { error = "The kit file is empty"; return false; }
    return true;
}

bool PulseForgeProcessor::saveDrum (int d, const File& f)
{
    XmlElement root ("PULSEFORGE_DRUM");
    root.setAttribute ("version", 1);
    root.setAttribute ("name", getDrumName (d));
    writeSound (root, getSound (d));
    return root.writeTo (f);
}

bool PulseForgeProcessor::loadDrum (int d, const File& f, String& error)
{
    auto xml = XmlDocument::parse (f);
    if (xml == nullptr || ! xml->hasTagName ("PULSEFORGE_DRUM")) { error = "That isn't a PulseForge drum file"; return false; }
    pushUndo();
    auto s = readSound (*xml, getSound (d));
    s[PF::Choke] = getSound (d)[PF::Choke];
    setSound (d, s);
    setDrumName (d, xml->getStringAttribute ("name", getDrumName (d)));
    return true;
}

//==============================================================================
// Undo
std::unique_ptr<XmlElement> PulseForgeProcessor::makeSnapshot() const
{
    auto x = std::make_unique<XmlElement> ("SNAP");
    writeKit (*x);
    writePatterns (*x);
    return x;
}

void PulseForgeProcessor::applySnapshot (const XmlElement& x)
{
    readKit (x);
    readPatterns (x);
}

void PulseForgeProcessor::pushUndo()
{
    undoStack.push_back (makeSnapshot());
    if (undoStack.size() > 40) undoStack.erase (undoStack.begin());
    redoStack.clear();
}

void PulseForgeProcessor::undo()
{
    if (undoStack.empty()) return;
    redoStack.push_back (makeSnapshot());
    auto s = std::move (undoStack.back());
    undoStack.pop_back();
    applySnapshot (*s);
}

void PulseForgeProcessor::redo()
{
    if (redoStack.empty()) return;
    undoStack.push_back (makeSnapshot());
    auto s = std::move (redoStack.back());
    redoStack.pop_back();
    applySnapshot (*s);
}

//==============================================================================
// Sequencer data
int PulseForgeProcessor::getStep (int p, int d, int s) const
{
    if (! isPositiveAndBelow (p, PF::NumPatterns) || ! isPositiveAndBelow (d, PF::NumDrums) || ! isPositiveAndBelow (s, PF::MaxSteps)) return 0;
    return steps[stepIndex (p, d, s)].load();
}

void PulseForgeProcessor::setStep (int p, int d, int s, int level)
{
    if (! isPositiveAndBelow (p, PF::NumPatterns) || ! isPositiveAndBelow (d, PF::NumDrums) || ! isPositiveAndBelow (s, PF::MaxSteps)) return;
    steps[stepIndex (p, d, s)] = (uint8) jlimit (0, 2, level);
}

int PulseForgeProcessor::getProb (int p, int d, int s) const
{
    if (! isPositiveAndBelow (p, PF::NumPatterns) || ! isPositiveAndBelow (d, PF::NumDrums) || ! isPositiveAndBelow (s, PF::MaxSteps)) return 100;
    return 100 - probs[stepIndex (p, d, s)].load();   // stored inverted so "0" (the default) means 100%
}

void PulseForgeProcessor::setProb (int p, int d, int s, int percent)
{
    if (! isPositiveAndBelow (p, PF::NumPatterns) || ! isPositiveAndBelow (d, PF::NumDrums) || ! isPositiveAndBelow (s, PF::MaxSteps)) return;
    probs[stepIndex (p, d, s)] = (uint8) (100 - jlimit (0, 100, percent));
}

void PulseForgeProcessor::setPatternLength (int p, int n)
{
    if (isPositiveAndBelow (p, PF::NumPatterns)) patLength[(size_t) p] = jlimit (1, PF::MaxSteps, n);
}

bool PulseForgeProcessor::laneIsEmpty (int p, int d) const
{
    for (int s = 0; s < PF::MaxSteps; ++s) if (getStep (p, d, s) > 0) return false;
    return true;
}

bool PulseForgeProcessor::patternIsEmpty (int p) const
{
    for (int d = 0; d < PF::NumDrums; ++d) if (! laneIsEmpty (p, d)) return false;
    return true;
}

void PulseForgeProcessor::clearLane (int p, int d)
{
    for (int s = 0; s < PF::MaxSteps; ++s) { setStep (p, d, s, 0); setProb (p, d, s, 100); }
}

void PulseForgeProcessor::clearPattern (int p)
{
    for (int d = 0; d < PF::NumDrums; ++d) clearLane (p, d);
}

void PulseForgeProcessor::randomizeLane (int p, int d)
{
    // density and accents that suit each drum's role
    static const float density[] = { 0.28f, 0.16f, 0.14f, 0.6f, 0.15f, 0.12f, 0.2f, 0.08f };
    const float dens = density[jlimit (0, 7, d)];
    const int len = getPatternLength (p);
    for (int s = 0; s < PF::MaxSteps; ++s)
    {
        int lvl = 0;
        if (s < len)
        {
            float chance = dens;
            if (d == 0 && s % 4 == 0) chance = 0.75f;           // kicks love the beat
            if ((d == 1 || d == 2) && s % 8 == 4) chance = 0.8f; // snares/claps love the backbeat
            if (rng.nextFloat() < chance) lvl = rng.nextFloat() < 0.25f ? 2 : 1;
        }
        setStep (p, d, s, lvl);
        setProb (p, d, s, lvl > 0 && rng.nextFloat() < 0.15f ? 50 : 100);
    }
}

void PulseForgeProcessor::randomizePattern (int p)
{
    for (int d = 0; d < PF::NumDrums; ++d) randomizeLane (p, d);
}

void PulseForgeProcessor::shiftLane (int p, int d, int amount)
{
    const int len = getPatternLength (p);
    std::array<int, PF::MaxSteps> lv {}, pr {};
    for (int s = 0; s < len; ++s) { lv[(size_t) s] = getStep (p, d, s); pr[(size_t) s] = getProb (p, d, s); }
    for (int s = 0; s < len; ++s)
    {
        const int from = ((s - amount) % len + len) % len;
        setStep (p, d, s, lv[(size_t) from]);
        setProb (p, d, s, pr[(size_t) from]);
    }
}

void PulseForgeProcessor::copyPattern (int from, int to)
{
    if (! isPositiveAndBelow (from, PF::NumPatterns) || ! isPositiveAndBelow (to, PF::NumPatterns) || from == to) return;
    for (int d = 0; d < PF::NumDrums; ++d)
        for (int s = 0; s < PF::MaxSteps; ++s)
        {
            setStep (to, d, s, getStep (from, d, s));
            setProb (to, d, s, getProb (from, d, s));
        }
    setPatternLength (to, getPatternLength (from));
}

StringArray PulseForgeProcessor::factoryPatternNames()
{
    StringArray s;
    for (auto& f : factoryPatterns) s.add (f.name);
    return s;
}

void PulseForgeProcessor::loadFactoryPattern (int p, int which)
{
    if (! isPositiveAndBelow (which, (int) std::size (factoryPatterns))) return;
    clearPattern (p);
    setPatternLength (p, 16);
    const auto& f = factoryPatterns[which];
    for (int d = 0; d < PF::NumDrums; ++d)
        for (int s = 0; s < 16 && f.rows[d][s] != 0; ++s)
        {
            const char c = f.rows[d][s];
            setStep (p, d, s, c == 'X' ? 2 : (c == 'x' || c == '?') ? 1 : 0);
            setProb (p, d, s, c == '?' ? 50 : 100);
        }
}

void PulseForgeProcessor::writePatterns (XmlElement& root) const
{
    auto* pats = root.createNewChildElement ("PATTERNS");
    for (int p = 0; p < PF::NumPatterns; ++p)
    {
        auto* pe = pats->createNewChildElement ("PATTERN");
        pe->setAttribute ("index", p);
        pe->setAttribute ("length", getPatternLength (p));
        for (int d = 0; d < PF::NumDrums; ++d)
        {
            String row;   // two characters per step: level digit, probability letter (A = 0% ... U = 100%)
            for (int s = 0; s < PF::MaxSteps; ++s)
            {
                row << String::charToString ((juce_wchar) ('0' + getStep (p, d, s)));
                row << String::charToString ((juce_wchar) ('A' + getProb (p, d, s) / 5));
            }
            pe->setAttribute ("d" + String (d), row);
        }
    }
}

void PulseForgeProcessor::readPatterns (const XmlElement& root)
{
    auto* pats = root.getChildByName ("PATTERNS");
    if (pats == nullptr) return;
    for (auto* pe : pats->getChildWithTagNameIterator ("PATTERN"))
    {
        const int p = pe->getIntAttribute ("index", -1);
        if (! isPositiveAndBelow (p, PF::NumPatterns)) continue;
        setPatternLength (p, pe->getIntAttribute ("length", 16));
        for (int d = 0; d < PF::NumDrums; ++d)
        {
            const String row = pe->getStringAttribute ("d" + String (d));
            for (int s = 0; s < PF::MaxSteps; ++s)
            {
                const int i = s * 2;
                const int lvl = i < row.length() ? (int) (row[i] - '0') : 0;
                const int pr = i + 1 < row.length() ? (int) (row[i + 1] - 'A') * 5 : 100;
                setStep (p, d, s, jlimit (0, 2, lvl));
                setProb (p, d, s, jlimit (0, 100, pr));
            }
        }
    }
}

//==============================================================================
// Audio engine
void PulseForgeProcessor::triggerDrum (int drum, int velocity)
{
    if (! isPositiveAndBelow (drum, PF::NumDrums)) return;
    const ScopedLock sl (triggerWriteLock);
    int s1, n1, s2, n2;
    triggerFifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) triggerData[(size_t) s1] = { drum, velocity };
    else if (n2 > 0) triggerData[(size_t) s2] = { drum, velocity };
    triggerFifo.finishedWrite (n1 + n2);
}

float PulseForgeProcessor::audioRandom01() noexcept
{
    audioRnd ^= audioRnd << 13; audioRnd ^= audioRnd >> 17; audioRnd ^= audioRnd << 5;
    return (float) (audioRnd >> 8) * (1.0f / 16777216.0f);
}

void PulseForgeProcessor::updateDrumParams()
{
    const float srf = (float) sr;
    auto coef = [srf] (float msTime) { return std::exp (std::log (0.001f) / jmax (1.0f, msTime * 0.001f * srf)); };

    for (int d = 0; d < PF::NumDrums; ++d)
    {
        auto& p = dp[(size_t) d];
        p.wave = jlimit (0, 3, (int) rp (d, RWave));
        p.pitch = jlimit (20.0, 12000.0, (double) rp (d, RPitch));
        p.bend = jlimit (-48.0, 48.0, (double) rp (d, RBend));
        p.bendCoef = coef (rp (d, RBendTime));
        p.oscCoef = coef (rp (d, ROscDecay));
        const float nd = jlimit (5.0f, 4000.0f, rp (d, RNoiseDecay));
        p.noiseCoef = coef (nd);
        p.noiseLinStep = 1.0f / jmax (1.0f, nd * 0.001f * srf);
        p.noiseHoldSamples = (int) (nd * 0.001f * srf);
        const float na = jlimit (0.0f, 300.0f, rp (d, RNoiseAttack));
        p.noiseAttackStep = na < 0.05f ? 1.0f : 1.0f / jmax (1.0f, na * 0.001f * srf);
        p.noiseShape = jlimit (0, 3, (int) rp (d, RNoiseShape));
        p.click = jlimit (0.0f, 1.0f, rp (d, RClick) * 0.01f);
        p.stereo = jlimit (0.0f, 1.0f, rp (d, RStereo) * 0.01f);
        p.mix = jlimit (0.0f, 1.0f, rp (d, RMix) * 0.01f);
        p.drive = jlimit (0.0f, 1.0f, rp (d, RDrive) * 0.01f);
        p.driveGain = 1.0f + p.drive * 15.0f;
        p.driveNorm = 1.0f / fastTanh (p.driveGain);
        p.filterMode = jlimit (0, 2, (int) rp (d, RFilterMode));

        const float fc = jlimit (20.0f, srf * 0.45f, rp (d, RCutoff));
        const float q = 0.5f * std::pow (30.0f, jlimit (0.0f, 1.0f, rp (d, RReso) * 0.01f));
        p.g = std::tan (MathConstants<float>::pi * fc / srf);
        p.k = 1.0f / q;
        p.a1 = 1.0f / (1.0f + p.g * (p.g + p.k));
        p.a2 = p.g * p.a1;
        p.a3 = p.g * p.a2;

        p.choke = jlimit (0, 3, (int) rp (d, RChoke));
        p.out = jlimit (0, PF::NumOuts - 1, (int) rp (d, ROut));
        p.muted = rp (d, RMute) > 0.5f;
        p.velSens = jlimit (0.0f, 1.0f, rp (d, RVelSens) * 0.01f);

        const float g = p.muted ? 0.0f : dbToGain (rp (d, RLevel));
        const float pan = jlimit (-1.0f, 1.0f, rp (d, RPan) * 0.01f);
        const float a = (pan + 1.0f) * MathConstants<float>::pi * 0.25f;
        p.gainL = g * std::cos (a) * MathConstants<float>::sqrt2;
        p.gainR = g * std::sin (a) * MathConstants<float>::sqrt2;
    }
}

void PulseForgeProcessor::startVoice (int drum, float vel)
{
    if (! isPositiveAndBelow (drum, PF::NumDrums)) return;
    const auto& p = dp[(size_t) drum];
    const float fastFade = 1.0f / (float) jmax (1.0, 0.004 * sr);

    for (auto& v : voices)
    {
        if (! v.active) continue;
        if (v.drum == drum && v.fadeStep == 0.0f) v.fadeStep = fastFade;   // retrigger: old hit fades out quickly
        else if (p.choke > 0 && v.drum != drum && dp[(size_t) v.drum].choke == p.choke) v.fadeStep = jmax (v.fadeStep, fastFade);
    }

    Voice* slot = nullptr;
    for (auto& v : voices) if (! v.active) { slot = &v; break; }
    if (slot == nullptr)   // all busy: take the quietest fading one
        for (auto& v : voices)
            if (slot == nullptr || (v.fadeStep > 0.0f && (slot->fadeStep == 0.0f || v.fade < slot->fade))) slot = &v;

    auto& v = *slot;
    v.active = true;
    v.drum = drum;
    v.phase = p.wave == 2 ? 0.5 : 0.0;   // start every wave near zero
    v.clickPhase = 0.0;
    v.oscEnv = 1.0f; v.pitchEnv = 1.0f; v.clickEnv = 1.0f;
    v.attack = p.noiseShape == 3 ? 1.0f : (p.noiseAttackStep >= 1.0f ? 1.0f : 0.0f);
    v.noiseEnv = v.attack >= 1.0f ? 1.0f : 0.0f;
    v.age = 0; v.noiseAge = 0;
    v.vel = 1.0f - p.velSens * (1.0f - jlimit (0.0f, 1.0f, vel));
    v.fade = 1.0f; v.fadeStep = 0.0f;
    v.gL = p.gainL; v.gR = p.gainR;
    v.ic1[0] = v.ic1[1] = v.ic2[0] = v.ic2[1] = 0.0f;
    drumHits[(size_t) drum].fetch_add (1);
}

void PulseForgeProcessor::renderVoices (int start, int num)
{
    if (num <= 0) return;
    const float isr = (float) (1.0 / sr);
    const float smooth = 1.0f - std::exp (-1.0f / (0.005f * (float) sr));
    const float clickCoef = std::exp (std::log (0.001f) / (0.004f * (float) sr));
    const float gateCoef = std::exp (std::log (0.001f) / (0.008f * (float) sr));
    const float burstCoef = std::exp (-1.0f / (0.0025f * (float) sr));
    const int burstLen = jmax (1, (int) (0.009 * sr));
    const float rampStep = 1.0f / jmax (1.0f, 0.00025f * (float) sr);
    const double maxF = sr * 0.45;

    for (auto& v : voices)
    {
        if (! v.active) continue;
        const auto& p = dp[(size_t) v.drum];
        float* L = busPtr[(size_t) p.out * 2] + start;
        float* R = busPtr[(size_t) p.out * 2 + 1] + start;
        const float oscAmt = 1.0f - p.mix, noiseAmt = p.mix;
        const float stN = 1.0f / std::sqrt ((1.0f - p.stereo) * (1.0f - p.stereo) + p.stereo * p.stereo);

        for (int i = 0; i < num; ++i)
        {
            // ---- tone oscillator with pitch envelope ----
            float osc = 0.0f;
            if (oscAmt > 0.0f && v.oscEnv > 1.0e-6f)
            {
                const double f = jlimit (1.0, maxF, p.pitch * std::exp (p.bend * (double) v.pitchEnv * 0.0577622650466621));
                const double dt = f / sr;
                v.phase += dt;
                if (v.phase >= 1.0) v.phase -= 1.0;
                const double ph = v.phase;
                switch (p.wave)
                {
                    case 0: osc = (float) std::sin (MathConstants<double>::twoPi * ph); break;
                    case 1: { double t = ph + 0.25; if (t >= 1.0) t -= 1.0; osc = (float) (4.0 * std::abs (t - 0.5) - 1.0); break; }
                    case 2: osc = (float) (2.0 * ph - 1.0) - polyBlep (ph, dt); break;
                    default:
                    {
                        osc = ph < 0.5 ? 1.0f : -1.0f;
                        osc += polyBlep (ph, dt);
                        double t2 = ph + 0.5; if (t2 >= 1.0) t2 -= 1.0;
                        osc -= polyBlep (t2, dt);
                        break;
                    }
                }
                const float ramp = jmin (1.0f, (float) v.age * rampStep);
                osc *= v.oscEnv * oscAmt * ramp;
            }

            // ---- filtered noise ----
            float nL = 0.0f, nR = 0.0f;
            if (noiseAmt > 0.0f && v.noiseEnv > 1.0e-6f)
            {
                const float w1 = noise (v.rnd);
                const float w2 = p.stereo > 0.0f ? noise (v.rnd2) : w1;
                const float in[2] = { w1, (w1 * (1.0f - p.stereo) + w2 * p.stereo) * stN };
                float o[2];
                for (int c = 0; c < 2; ++c)
                {
                    const float x = in[c];
                    const float v3 = x - v.ic2[c];
                    const float v1 = p.a1 * v.ic1[c] + p.a2 * v3;
                    const float v2 = v.ic2[c] + p.a2 * v.ic1[c] + p.a3 * v3;
                    v.ic1[c] = 2.0f * v1 - v.ic1[c];
                    v.ic2[c] = 2.0f * v2 - v.ic2[c];
                    o[c] = p.filterMode == 0 ? v2 : (p.filterMode == 1 ? p.k * v1 : x - p.k * v1 - v2);
                }
                const float e = v.noiseEnv * noiseAmt;
                nL = o[0] * e; nR = o[1] * e;
            }

            // ---- click transient ----
            float clk = 0.0f;
            if (p.click > 0.0f && v.clickEnv > 1.0e-5f)
            {
                v.clickPhase += 1800.0 * (double) isr;
                if (v.clickPhase >= 1.0) v.clickPhase -= 1.0;
                clk = (0.5f * noise (v.rnd2) + 0.8f * (float) std::sin (MathConstants<double>::twoPi * v.clickPhase)) * v.clickEnv * p.click;
            }

            float sL = osc + nL + clk, sR = osc + nR + clk;
            if (p.drive > 0.001f)
            {
                sL = fastTanh (sL * p.driveGain) * p.driveNorm;
                sR = fastTanh (sR * p.driveGain) * p.driveNorm;
            }

            v.gL += (p.gainL - v.gL) * smooth;
            v.gR += (p.gainR - v.gR) * smooth;
            float amp = v.vel;
            if (v.fadeStep > 0.0f)
            {
                v.fade -= v.fadeStep;
                if (v.fade <= 0.0f) { v.active = false; break; }
                amp *= v.fade;
            }
            L[i] += sL * v.gL * amp;
            R[i] += sR * v.gR * amp;

            // ---- envelopes ----
            v.oscEnv *= p.oscCoef;
            v.pitchEnv *= p.bendCoef;
            v.clickEnv *= clickCoef;
            if (p.noiseShape == 3)   // clap: three quick bursts, then the tail
            {
                if (v.age < burstLen * 3) v.noiseEnv = (v.age % burstLen == 0) ? 1.0f : v.noiseEnv * burstCoef;
                else if (v.age == burstLen * 3) v.noiseEnv = 1.0f;
                else v.noiseEnv *= p.noiseCoef;
            }
            else if (v.attack < 1.0f)
            {
                v.attack = jmin (1.0f, v.attack + p.noiseAttackStep);
                v.noiseEnv = v.attack;
            }
            else if (p.noiseShape == 1) v.noiseEnv = jmax (0.0f, v.noiseEnv - p.noiseLinStep);
            else if (p.noiseShape == 2) { if (v.noiseAge > p.noiseHoldSamples) v.noiseEnv *= gateCoef; }
            else v.noiseEnv *= p.noiseCoef;
            ++v.age;
            if (v.attack >= 1.0f) ++v.noiseAge;   // the decay stage counts from the end of the attack

            // ---- finished? ----
            if ((v.age & 31) == 0 && v.attack >= 1.0f && ! (p.noiseShape == 3 && v.age <= burstLen * 3)
                && ! (p.noiseShape == 2 && v.noiseAge <= p.noiseHoldSamples))
            {
                const float level = v.oscEnv * oscAmt + v.noiseEnv * noiseAmt + v.clickEnv * p.click;
                if (level < 1.0e-5f) { v.active = false; break; }
            }
        }
    }
}

void PulseForgeProcessor::addEvent (int offset, int drum, float vel)
{
    if (numEvents < (int) events.size()) events[(size_t) numEvents++] = { offset, drum, vel };
}

void PulseForgeProcessor::sequence (int n)
{
    if (rawSeqOn->load() < 0.5f) { displayStep = -1; wasRunning = false; return; }

    bool hostPlaying = false;
    double bpm = 120.0, hostPpq = 0.0;
    bool havePpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            hostPlaying = pos->getIsPlaying();
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) { hostPpq = *q; havePpq = true; }
        }
    if (! std::isfinite (bpm) || bpm < 20.0 || bpm > 999.0) bpm = 120.0;

    const bool useHost = hostPlaying && havePpq && std::isfinite (hostPpq);
    const bool useFree = ! useHost && internalPlay.load();
    if (! useHost && ! useFree) { displayStep = -1; wasRunning = false; return; }

    const double spb = sr * 60.0 / bpm;
    if (useFree && ! wasRunning) freePpq = 0.0;
    const double start = useHost ? hostPpq : freePpq;
    const double end = start + n / spb;
    if (useFree) freePpq = end;
    wasRunning = true;

    const int pat = jlimit (0, PF::NumPatterns - 1, currentPattern.load());
    const int len = jlimit (1, PF::MaxSteps, patLength[(size_t) pat].load());
    const double stl = PF::rateBeats ((int) rawRate->load());
    const double swd = jlimit (0.0f, 100.0f, rawSwing->load()) / 100.0 * 0.5 * stl;

    const int64 kFirst = (int64) std::floor (start / stl) - 1, kLast = (int64) std::floor (end / stl);
    for (int64 k = kFirst; k <= kLast; ++k)
    {
        const double t = (double) k * stl + ((k & 1) != 0 ? swd : 0.0);
        if (t < start || t >= end) continue;
        const int offset = jlimit (0, n - 1, (int) ((t - start) * spb));
        const int step = (int) (((k % len) + len) % len);
        displayStep = step;
        for (int d = 0; d < PF::NumDrums; ++d)
        {
            const size_t i = stepIndex (pat, d, step);
            const int lvl = steps[i].load();
            if (lvl <= 0) continue;
            const int prob = 100 - probs[i].load();
            if (prob < 100 && audioRandom01() * 100.0f >= (float) prob) continue;
            addEvent (offset, d, lvl == 2 ? 1.0f : 0.72f);
        }
    }
}

void PulseForgeProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0 || buffer.getNumChannels() < 1) { midi.clear(); return; }   // some hosts send empty blocks
    updateDrumParams();

    // output pointers per drum output (disabled outputs fall back to Out 1)
    {
        auto main = getBusBuffer (buffer, false, 0);
        float* mL = main.getNumChannels() > 0 ? main.getWritePointer (0) : buffer.getWritePointer (0);
        float* mR = main.getNumChannels() > 1 ? main.getWritePointer (1) : mL;
        for (int b = 0; b < PF::NumOuts; ++b)
        {
            float* L = mL; float* R = mR;
            if (b > 0 && b < getBusCount (false))
            {
                auto bb = getBusBuffer (buffer, false, b);
                if (bb.getNumChannels() >= 1) { L = bb.getWritePointer (0); R = bb.getNumChannels() > 1 ? bb.getWritePointer (1) : L; }
            }
            busPtr[(size_t) b * 2] = L;
            busPtr[(size_t) b * 2 + 1] = R;
        }
    }

    numEvents = 0;

    // MIDI in: C1-G1 play the drums, C2-G2 switch patterns
    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        if (! msg.isNoteOn()) continue;
        const int note = msg.getNoteNumber();
        const int off = jlimit (0, n - 1, m.samplePosition);
        if (note >= PF::DrumNote && note < PF::DrumNote + PF::NumDrums)
            addEvent (off, note - PF::DrumNote, msg.getVelocity() / 127.0f);
        else if (note >= PF::PatternNote && note < PF::PatternNote + PF::NumPatterns)
            currentPattern = note - PF::PatternNote;
    }
    midi.clear();

    // drum clicks from the editor
    {
        int s1, n1, s2, n2;
        triggerFifo.prepareToRead (triggerFifo.getNumReady(), s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) addEvent (0, triggerData[(size_t) (s1 + i)].first, triggerData[(size_t) (s1 + i)].second / 127.0f);
        for (int i = 0; i < n2; ++i) addEvent (0, triggerData[(size_t) (s2 + i)].first, triggerData[(size_t) (s2 + i)].second / 127.0f);
        triggerFifo.finishedRead (n1 + n2);
    }

    sequence (n);

    std::sort (events.begin(), events.begin() + numEvents, [] (const Event& a, const Event& b) { return a.offset < b.offset; });
    int pos = 0;
    for (int e = 0; e < numEvents; ++e)
    {
        const auto& ev = events[(size_t) e];
        renderVoices (pos, ev.offset - pos);
        pos = jmax (pos, ev.offset);
        startVoice (ev.drum, ev.vel);
    }
    renderVoices (pos, n - pos);

    buffer.applyGain (dbToGain (jlimit (-24.0f, 6.0f, rawMaster->load())));

    // Speaker/ear protection: if anything went wrong (NaN, infinity, runaway
    // level), mute that output for this block and reset the voices instead of
    // sending a blast down the chain.
    bool broken = false;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            if (! std::isfinite (d[i])) { buffer.clear (ch, 0, n); broken = true; break; }
            d[i] = jlimit (-outputCeiling, outputCeiling, d[i]);
        }
    }
    if (broken) resetVoices();
}

//==============================================================================
// REAPER multi-out
bool PulseForgeProcessor::createMultiOuts (bool onlyUsed, String& message)
{
    if (! isInReaper()) { message = "Multi Out needs REAPER (route the 8 outputs by hand in other hosts)"; return false; }

    // first time: give every drum its own output
    bool allOnOne = true;
    for (int d = 0; d < PF::NumDrums; ++d) allOnOne &= drumOut (d) == 0;
    if (allOnOne)
        for (int d = 0; d < PF::NumDrums; ++d) setParam (PF::pid (d, "out"), (float) d);

    std::array<ReaperBridge::OutputInfo, 16> outs;
    for (int o = 0; o < PF::NumOuts; ++o)
    {
        StringArray ns;
        int first = -1;
        for (int d = 0; d < PF::NumDrums; ++d)
        {
            if (drumOut (d) != o) continue;
            ns.add (getDrumName (d));
            if (first < 0) first = d;
        }
        auto& out = outs[(size_t) o];
        out.wanted = ! onlyUsed || ! ns.isEmpty();
        out.name = ns.isEmpty() ? "PulseForge Out " + String (o + 1) : ns.joinIntoString (" + ");
        out.colour = first >= 0 ? (PF::drumColour (first).getARGB() & 0xffffff) : 0;
    }
    const bool ok = reaper.createMultiOutTracks (outs, PF::NumOuts * 2, message);
    if (ok && allOnOne) message << " - each drum now has its own output";
    return ok;
}

//==============================================================================
// MIDI clip of the current pattern (for dragging onto a track)
File PulseForgeProcessor::writeMidiFile (int onlyDrum)
{
    constexpr int tpq = 960;
    const int p = jlimit (0, PF::NumPatterns - 1, currentPattern.load());
    const int len = getPatternLength (p);
    const double stl = PF::rateBeats ((int) param ("rate"));
    const double L = len * stl;
    const double swd = param ("swing") / 100.0 * 0.5 * stl;
    static const int bars[] = { 0, 1, 2, 4, 8 };
    const double total = clipLength == 0 ? L : bars[jlimit (0, 4, clipLength)] * 4.0;

    MidiMessageSequence seq;
    const String patName = String::charToString ((juce_wchar) ('A' + p));
    seq.addEvent (MidiMessage::textMetaEvent (3, "PulseForge " + patName + (onlyDrum >= 0 ? " " + getDrumName (onlyDrum) : String())), 0);
    seq.addEvent (MidiMessage::timeSignatureMetaEvent (4, 4), 0);

    for (double c = 0.0; c * L < total - 1.0e-9; c += 1.0)
        for (int s = 0; s < len; ++s)
        {
            const double t = c * L + s * stl + ((s & 1) ? swd : 0.0);
            if (t >= total - 1.0e-9) continue;
            for (int d = 0; d < PF::NumDrums; ++d)
            {
                if (onlyDrum >= 0 && d != onlyDrum) continue;
                if (onlyDrum < 0 && param (PF::pid (d, "mute")) > 0.5f) continue;
                const int lvl = getStep (p, d, s);
                if (lvl <= 0) continue;
                const int prob = getProb (p, d, s);
                if (prob < 100 && rng.nextInt (100) >= prob) continue;   // the clip gets one roll of the dice per hit
                const double on = std::round (t * tpq);
                const double off = std::min (std::round (total * tpq), std::max (on + 1.0, std::round ((t + stl * 0.5) * tpq)));
                seq.addEvent (MidiMessage::noteOn (1, PF::DrumNote + d, (uint8) (lvl == 2 ? 127 : 92)), on);
                seq.addEvent (MidiMessage::noteOff (1, PF::DrumNote + d), off);
            }
        }
    seq.addEvent (MidiMessage::endOfTrack(), std::round (total * tpq));
    seq.updateMatchedPairs();

    MidiFile file;
    file.setTicksPerQuarterNote (tpq);
    file.addTrack (seq);

    auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("PulseForge");
    dir.createDirectory();
    auto out = dir.getChildFile (File::createLegalFileName ("PulseForge " + patName
                                                            + (onlyDrum >= 0 ? " " + getDrumName (onlyDrum) : String())
                                                            + " " + Time::getCurrentTime().formatted ("%H%M%S") + ".mid"));
    FileOutputStream stream (out);
    if (stream.openedOk())
    {
        stream.setPosition (0);
        stream.truncate();
        file.writeTo (stream, 1);
        stream.flush();
    }
    return out;
}

//==============================================================================
// State
void PulseForgeProcessor::getStateInformation (MemoryBlock& destData)
{
    XmlElement root ("PULSEFORGE");
    root.setAttribute ("version", 1);
    root.setAttribute ("editorW", editorWidth);
    root.setAttribute ("editorH", editorHeight);
    root.setAttribute ("selDrum", selectedDrum);
    root.setAttribute ("pattern", currentPattern.load());
    root.setAttribute ("clipLen", clipLength);
    root.setAttribute ("probMode", probMode);
    root.setAttribute ("mutate", mutateAmount);
    {
        auto* ne = root.createNewChildElement ("NAMES");
        for (int d = 0; d < PF::NumDrums; ++d) ne->setAttribute ("n" + String (d), getDrumName (d));
    }
    writePatterns (root);
    if (auto params = apvts.copyState().createXml())
        root.addChildElement (params.release());
    copyXmlToBinary (root, destData);
}

void PulseForgeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0) return;
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName ("PULSEFORGE")) return;

    editorWidth = jlimit (880, 2400, xml->getIntAttribute ("editorW", editorWidth));
    editorHeight = jlimit (580, 1600, xml->getIntAttribute ("editorH", editorHeight));
    selectedDrum = jlimit (0, PF::NumDrums - 1, xml->getIntAttribute ("selDrum", 0));
    currentPattern = jlimit (0, PF::NumPatterns - 1, xml->getIntAttribute ("pattern", 0));
    clipLength = jlimit (0, 4, xml->getIntAttribute ("clipLen", 0));
    probMode = xml->getBoolAttribute ("probMode", false);
    mutateAmount = jlimit (0.0f, 1.0f, (float) xml->getDoubleAttribute ("mutate", 0.35));

    if (auto* ne = xml->getChildByName ("NAMES"))
        for (int d = 0; d < PF::NumDrums; ++d)
            if (ne->hasAttribute ("n" + String (d))) setDrumName (d, ne->getStringAttribute ("n" + String (d)));

    readPatterns (*xml);

    if (auto* params = xml->getChildByName (apvts.state.getType()))
        apvts.replaceState (ValueTree::fromXml (*params));

    undoStack.clear();
    redoStack.clear();
}

//==============================================================================
AudioProcessorEditor* PulseForgeProcessor::createEditor() { return new PulseForgeEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PulseForgeProcessor(); }
