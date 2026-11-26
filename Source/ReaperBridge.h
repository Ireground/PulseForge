#pragma once
// Talks to REAPER through its VST3 extension interface. In other hosts every
// call simply reports "not REAPER" and does nothing.

#include <juce_core/juce_core.h>
#include <array>

namespace Steinberg { class FUnknown; }

class ReaperBridge
{
public:
    ReaperBridge() = default;
    ~ReaperBridge();

    void setHostContext (Steinberg::FUnknown* hostContext);
    bool isReaper() const noexcept { return host != nullptr; }

    // Folder containing the saved .rpp file. Invalid File if unknown/unsaved.
    juce::File getProjectFolder() const;

    // Multi-out routing (Sitala style). For every requested output pair a track
    // is created right below the PulseForge track with a receive from that pair.
    // Already-routed outputs are kept and only renamed/recoloured.
    struct OutputInfo
    {
        bool wanted = false;
        juce::String name;
        juce::uint32 colour = 0;   // 0xRRGGBB, 0 = leave default
    };
    bool createMultiOutTracks (const std::array<OutputInfo, 16>& outs, int numChannels,
                               juce::String& message);

private:
    void* api (const char* name) const;
    void* host = nullptr;   // reaper::IReaperHostApplication*
    JUCE_DECLARE_NON_COPYABLE (ReaperBridge)
};
