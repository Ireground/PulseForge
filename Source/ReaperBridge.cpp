#include "ReaperBridge.h"
#include <pluginterfaces/base/funknown.h>
#include <map>

namespace
{
    // Layout from REAPER SDK's reaper_vst3_interfaces.h (REAPER 5.02+)
    class IReaperHostApplication : public Steinberg::FUnknown
    {
    public:
        virtual void* PLUGIN_API getReaperApi (Steinberg::CStringA funcname) = 0;
        virtual void* PLUGIN_API getReaperParent (Steinberg::uint32 w) = 0; // 1 track, 2 take, 3 project
        virtual void* PLUGIN_API reaperExtended (Steinberg::uint32 call, void* p1, void* p2, void* p3) = 0;
    };

    const Steinberg::TUID reaperHostIID = INLINE_UID (0x79655E36, 0x77EE4267, 0xA573FEF7, 0x4912C27C);

    IReaperHostApplication* asReaper (void* p) { return static_cast<IReaperHostApplication*> (p); }
}

ReaperBridge::~ReaperBridge()
{
    if (host != nullptr)
        asReaper (host)->release();
}

void ReaperBridge::setHostContext (Steinberg::FUnknown* hostContext)
{
    if (hostContext == nullptr || host != nullptr)
        return;

    void* obj = nullptr;
    if (hostContext->queryInterface (reaperHostIID, &obj) == Steinberg::kResultOk && obj != nullptr)
        host = obj;   // keep the reference; released in destructor
}

void* ReaperBridge::api (const char* name) const
{
    return host != nullptr ? asReaper (host)->getReaperApi (name) : nullptr;
}

juce::File ReaperBridge::getProjectFolder() const
{
    if (host == nullptr)
        return {};

    using EnumProjectsFn = void* (*) (int idx, char* fn, int fnSize);
    auto enumProjects = reinterpret_cast<EnumProjectsFn> (api ("EnumProjects"));
    if (enumProjects == nullptr)
        return {};

    void* myProject = asReaper (host)->getReaperParent (3);
    char buf[4096];

    auto folderFrom = [] (const char* path) -> juce::File
    {
        juce::String s = juce::String::fromUTF8 (path);
        if (s.isEmpty() || ! juce::File::isAbsolutePath (s)) return {};
        return juce::File (s).getParentDirectory();
    };

    if (myProject != nullptr)
    {
        for (int i = 0; i < 512; ++i)
        {
            buf[0] = 0;
            void* p = enumProjects (i, buf, (int) sizeof (buf));
            if (p == nullptr) break;
            if (p == myProject) return folderFrom (buf);
        }
        return {};
    }

    buf[0] = 0;
    if (enumProjects (-1, buf, (int) sizeof (buf)) != nullptr)
        return folderFrom (buf);
    return {};
}

bool ReaperBridge::createMultiOutTracks (const std::array<OutputInfo, 16>& outs, int numChannels, juce::String& message)
{
    if (host == nullptr) { message = "Multi Out only works in REAPER"; return false; }
    auto* h = asReaper (host);

    using GetTrackFn       = void* (*) (void* proj, int idx);
    using InsertTrackFn    = void  (*) (int idx, bool wantDefaults);
    using GetInfoFn        = double (*) (void* tr, const char* parm);
    using SetInfoFn        = bool  (*) (void* tr, const char* parm, double v);
    using SetStrFn         = bool  (*) (void* tr, const char* parm, char* buf, bool set);
    using CreateSendFn     = int   (*) (void* tr, void* dest);
    using NumSendsFn       = int   (*) (void* tr, int category);
    using SendValFn        = double (*) (void* tr, int category, int idx, const char* parm);
    using SetSendValFn     = bool  (*) (void* tr, int category, int idx, const char* parm, double v);
    using SendInfoFn       = void* (*) (void* tr, int category, int idx, const char* parm, void* set);
    using ColorToNativeFn  = int   (*) (int r, int g, int b);
    using UndoBeginFn      = void  (*) (void* proj);
    using UndoEndFn        = void  (*) (void* proj, const char* desc, int flags);
    using IntFn            = void  (*) (int);
    using BoolFn           = void  (*) (bool);
    using VoidFn           = void  (*) ();

    auto getTrack      = reinterpret_cast<GetTrackFn>      (api ("GetTrack"));
    auto insertTrack   = reinterpret_cast<InsertTrackFn>   (api ("InsertTrackAtIndex"));
    auto getInfo       = reinterpret_cast<GetInfoFn>       (api ("GetMediaTrackInfo_Value"));
    auto setInfo       = reinterpret_cast<SetInfoFn>       (api ("SetMediaTrackInfo_Value"));
    auto setStr        = reinterpret_cast<SetStrFn>        (api ("GetSetMediaTrackInfo_String"));
    auto createSend    = reinterpret_cast<CreateSendFn>    (api ("CreateTrackSend"));
    auto numSends      = reinterpret_cast<NumSendsFn>      (api ("GetTrackNumSends"));
    auto sendVal       = reinterpret_cast<SendValFn>       (api ("GetTrackSendInfo_Value"));
    auto setSendVal    = reinterpret_cast<SetSendValFn>    (api ("SetTrackSendInfo_Value"));
    auto sendInfo      = reinterpret_cast<SendInfoFn>      (api ("GetSetTrackSendInfo"));
    auto colorToNative = reinterpret_cast<ColorToNativeFn> (api ("ColorToNative"));
    auto undoBegin     = reinterpret_cast<UndoBeginFn>     (api ("Undo_BeginBlock2"));
    auto undoEnd       = reinterpret_cast<UndoEndFn>       (api ("Undo_EndBlock2"));
    auto preventUI     = reinterpret_cast<IntFn>           (api ("PreventUIRefresh"));
    auto adjustTracks  = reinterpret_cast<BoolFn>          (api ("TrackList_AdjustWindows"));
    auto updateArrange = reinterpret_cast<VoidFn>          (api ("UpdateArrange"));

    if (getTrack == nullptr || insertTrack == nullptr || getInfo == nullptr || setInfo == nullptr || setStr == nullptr
        || createSend == nullptr || numSends == nullptr || sendVal == nullptr || setSendVal == nullptr || sendInfo == nullptr)
    {
        message = "This REAPER version is too old for Multi Out";
        return false;
    }

    void* proj = h->getReaperParent (3);
    void* myTrack = h->getReaperParent (1);
    if (myTrack == nullptr) { message = "Could not find the PulseForge track"; return false; }

    const int myNumber = (int) getInfo (myTrack, "IP_TRACKNUMBER");   // 1-based
    if (myNumber <= 0) { message = "PulseForge must be on a normal track (not the master)"; return false; }
    if (getInfo (myTrack, "I_FOLDERDEPTH") > 0.5)
    {
        message = "The PulseForge track is a folder. Put it on a normal track first";
        return false;
    }

    // Which output pairs already have a receive track?
    std::map<int, void*> existing;   // pair index -> destination track
    int lastExistingNumber = myNumber;
    for (int i = 0, n = numSends (myTrack, 0); i < n; ++i)
    {
        const int src = (int) sendVal (myTrack, 0, i, "I_SRCCHAN");
        if (src < 0 || (src & 1) != 0 || src >= 1024) continue;   // only stereo pairs
        void* dest = sendInfo (myTrack, 0, i, "P_DESTTRACK", nullptr);
        if (dest == nullptr) continue;
        existing[src / 2] = dest;
        lastExistingNumber = juce::jmax (lastExistingNumber, (int) getInfo (dest, "IP_TRACKNUMBER"));
    }

    auto applyLook = [&] (void* tr, const OutputInfo& o)
    {
        juce::String name = o.name;
        juce::HeapBlock<char> buf (512, true);
        name.copyToUTF8 (buf.getData(), 511);
        setStr (tr, "P_NAME", buf.getData(), true);
        if (o.colour != 0 && colorToNative != nullptr)
        {
            const int c = colorToNative ((int) ((o.colour >> 16) & 0xff), (int) ((o.colour >> 8) & 0xff), (int) (o.colour & 0xff));
            setInfo (tr, "I_CUSTOMCOLOR", (double) (c | 0x1000000));
        }
    };

    if (undoBegin != nullptr) undoBegin (proj);
    if (preventUI != nullptr) preventUI (1);

    if (getInfo (myTrack, "I_NCHAN") < (double) numChannels)
        setInfo (myTrack, "I_NCHAN", (double) numChannels);

    int created = 0, renamed = 0;
    int insertIndex = lastExistingNumber;   // 0-based index of the slot after that track

    for (int pair = 0; pair < 16; ++pair)
    {
        const auto& o = outs[(size_t) pair];
        auto it = existing.find (pair);
        if (it != existing.end())
        {
            applyLook (it->second, o);
            ++renamed;
            continue;
        }
        if (! o.wanted) continue;

        // keep folder structure: the track we insert after hands over its "folder end" flag
        void* prev = getTrack (proj, insertIndex - 1);
        const double prevDepth = prev != nullptr ? getInfo (prev, "I_FOLDERDEPTH") : 0.0;

        insertTrack (insertIndex, true);
        void* tr = getTrack (proj, insertIndex);
        if (tr == nullptr) continue;
        ++insertIndex;

        if (prevDepth < -0.5)
        {
            setInfo (prev, "I_FOLDERDEPTH", 0.0);
            setInfo (tr, "I_FOLDERDEPTH", prevDepth);
        }

        applyLook (tr, o);
        setInfo (tr, "I_NCHAN", 2.0);
        const int s = createSend (myTrack, tr);
        if (s >= 0)
        {
            setSendVal (myTrack, 0, s, "I_SRCCHAN", (double) (pair * 2));
            setSendVal (myTrack, 0, s, "I_DSTCHAN", 0.0);
            setSendVal (myTrack, 0, s, "I_SENDMODE", 0.0);   // post-fader
            setSendVal (myTrack, 0, s, "D_VOL", 1.0);
            setSendVal (myTrack, 0, s, "D_PAN", 0.0);
        }
        ++created;
    }

    // the outputs now carry the sound: stop the drum track from also playing outs 1/2 directly
    if (created + renamed > 0)
        setInfo (myTrack, "B_MAINSEND", 0.0);

    if (preventUI != nullptr) preventUI (-1);
    if (adjustTracks != nullptr) adjustTracks (false);
    if (updateArrange != nullptr) updateArrange();
    if (undoEnd != nullptr) undoEnd (proj, "PulseForge: create multi-out tracks", -1);

    if (created == 0 && renamed > 0)
        message = "Outputs already routed - track names and colours updated";
    else if (created > 0)
        message = "Created " + juce::String (created) + " output track" + (created == 1 ? "" : "s")
                  + (renamed > 0 ? " (updated " + juce::String (renamed) + " existing)" : juce::String());
    else
        message = "Nothing to create";
    return created + renamed > 0;
}
