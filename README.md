# PulseForge

An 8-voice synthetic drum machine (VST3). Every sound is synthesized in real time from an oscillator, filtered noise and a click; no samples are used. It has a built-in 32-step sequencer, multi-out with one-click REAPER routing, and drag-to-MIDI.

## Install
1. Open the **Actions** tab, click the latest green run, and download **PulseForge-VST3-Windows**.
2. Unzip it and copy the `PulseForge.vst3` folder to `C:\Program Files\Common Files\VST3`.
3. In REAPER: Options → Preferences → Plug-ins → VST → **Re-scan**.

## The basics
- **Click a drum name** to hear it and edit its sound below. Double-click the big name to rename it.
- **Steps:** click cycles on → accent → off. Drag to paint. Right-click clears.
- **CHANCE mode:** drag up and down on a step to set how often it plays (5–100 %).
- **Patterns A–H.** MIDI notes 36–43 play the drums, 48–55 switch patterns live.
- **PLAY** plays the pattern any time (in sync when REAPER is playing).
- **Play with REAPER:** on = the pattern plays whenever REAPER plays. Off = PulseForge plays only from MIDI and the PLAY button.

## Sounds
- **TONE:** wave, pitch, bend (pitch sweep), bend time, decay, click.
- **NOISE:** filter type, envelope (Exponential / Linear / Gated / Clap), cutoff, resonance, attack, decay, stereo width.
- **OUTPUT:** tone/noise mix, drive, level, pan, velocity sensitivity, output, choke group.
- **Knobs:** drag up/down (or left/right). Hold **Shift** while dragging or using the mouse wheel for fine adjustments. Double-click resets a knob.
- **RANDOM** makes a new sound that still fits the drum's role. **MUTATE** nudges it by the AMOUNT knob. **RESET** goes back to the Classic kit.
- **KITS:** 5 factory kits (Classic, EDM Punch, Techno Warehouse, 808 Style, Glitch Lab), plus save/load of your own `.pfkit`. Single drums save as `.pfdrum`. Your files go to `Documents\PulseForge`.
- **UNDO / REDO** covers kit and pattern changes. REAPER's own undo covers single knob moves.

## Multi-out (REAPER)
- **MULTI OUT** creates one track per output, named and coloured after its drums, with the routing done.
- The first time, every drum gets its own output. After that, set each drum's **OUTPUT** box to choose; several drums can share one.
- **...** next to it has *only outputs in use*, *each drum its own output* and *all on Out 1*.

## Drag to MIDI
- **DRAG MIDI** → drop on a track to get the current pattern as a MIDI clip. The clip-length box sets 1–8 bars. Afterwards *Play with REAPER* switches itself off so hits don't play twice; PLAY still works for auditioning.
- **Drag a drum name** onto a track to get just that row.
- Steps with less than 100 % chance roll the dice once when the clip is made.

## Safety
- Output guard: NaN/infinite values are silenced and the voices reset; nothing gets above +18 dBFS.
- Denormal protection, smoothed level/pan changes, nothing on the audio thread that allocates memory or waits on a lock.
- Every build is tested with pluginval (strictness 5) on GitHub.
