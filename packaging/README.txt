ONE KNOB  -  version @VERSION@
by fenrikos

A one-knob saturator with three flavours: Dwarven (tape), Elven (console) and Dragonfire (destroy).
Turn the knob from a gentle warmth to total ruin; the output stays as loud as the input all the way.


REQUIREMENTS
------------
[linux]
- 64-bit Linux (x86_64), glibc @GLIBC@ or newer
- A VST3 or LV2 host (REAPER, Bitwig Studio, Ardour, Carla, Qtractor, ...) or use the Standalone app
- X11 or XWayland for the window
[/linux]
[windows]
- Windows 10 or 11, 64-bit
- A VST3 host (REAPER, Ableton Live, FL Studio, Cubase, Studio One, Bitwig Studio, ...) or use the Standalone app
[/windows]
[macos]
- macOS 10.15 or newer, Apple Silicon or Intel (universal binaries)
- An AU or VST3 host (Logic Pro, GarageBand, Ableton Live, REAPER, ...) or use the Standalone app
[/macos]


INSTALL
-------
[linux]
Open a terminal in this folder and run:

    ./install.sh             installs for your user (~/.vst3 and ~/.lv2)
    ./install.sh --system    installs for all users (/usr/lib/vst3, /usr/lib/lv2; asks for sudo)

Or copy "VST3/One Knob.vst3" into ~/.vst3/ and "LV2/One Knob.lv2" into ~/.lv2/ by hand.
The Standalone app runs straight from "Standalone/One Knob".
To remove it, run ./uninstall.sh (add --system if you installed system-wide).
[/linux]
[windows]
Copy the "One Knob.vst3" folder inside VST3\ into:

    C:\Program Files\Common Files\VST3\

Windows asks for administrator permission to copy there. The Standalone app (Standalone\One Knob.exe)
runs from anywhere. To remove One Knob, delete those files.
[/windows]
[macos]
1. Copy the plugins:
     AU/One Knob.component   ->  /Library/Audio/Plug-Ins/Components/
     VST3/One Knob.vst3      ->  /Library/Audio/Plug-Ins/VST3/
   The Standalone app (Standalone/One Knob.app) can go in Applications.
   (In Finder: Go > Go to Folder..., then paste the path.)
@MAC_QUARANTINE@
To remove One Knob, delete those files again.
[/macos]

Then rescan plugins in your host. In REAPER: Options > Preferences > Plug-ins > "Re-scan".
[macos]
Logic Pro and GarageBand pick up new AU plugins after a restart.
[/macos]

Upgrading from OneKnobToRuleThemAll 1.0: One Knob replaces it and opens your old sessions with the
same settings. Delete the old OneKnobToRuleThemAll plugin files so your host does not list it twice.


USING ONE KNOB
--------------
- Drag the knob up or down (or use the mouse wheel). Double-click it to go back to 0 %.
- At 0 % the sound passes through untouched; the effect fades in over the first few percent.
- Click DWARVEN, ELVEN or DRAGONFIRE to change the flavour. Switching is click-free.
- Hosts also see a Bypass parameter, which crossfades to the dry signal without clicks.
- One Knob adds about 1.4 ms of latency (65 samples at 48 kHz, for its 8x oversampling) and reports it
  to your host, which compensates automatically.


LICENCE AND SOURCE
------------------
Copyright (C) 2026 fenrikos. One Knob is free software under the GNU Affero General Public License,
version 3 or later (see LICENSE.txt). It comes with ABSOLUTELY NO WARRANTY.
The complete source code is at https://github.com/FenrirRhogar/OneKnob
