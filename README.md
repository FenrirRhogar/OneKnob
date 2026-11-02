<div align="center">
  <img src="Assets/Untitled (600 x 600 px).png" alt="OneKnobToRuleThemAll Logo" width="200"/>
  <h1>OneKnobToRuleThemAll</h1>
  <p><b>A Distortion & Saturation Plugin That is Legally Distinct From Any Ring-Obsessed Fantasy Franchises</b></p>
</div>

---

<div align="center">
  <img src="OneKnobToRuleThemAll_screenshot.png" alt="OneKnobToRuleThemAll Screenshot" width="400"/>
</div>

## 🎛️ Overview

**OneKnobToRuleThemAll** is a single-knob saturation and distortion audio plugin built with JUCE. It features a completely custom aesthetic that successfully avoids the gaze of large corporate lawyers (and other, more fiery, lidless eyes).

## ✨ Key Features

- **One Knob Design**: Turn the single, precious knob up to dial in increasingly heavy distortion.
- **3 Distinct Flavors**:
  - **DWARVEN**: Tape saturation style. Digs too greedily and too deep.
  - **ELVEN**: Smooth analog console overdrive. Elegant and immortal.
  - **MORDOR**: Total destruction and hard clipping. One does not simply walk into this mode.
- **Dynamic UI**: Text and colors dynamically change and respond as the drive increases.
- **Zero Latency DSP**: High-performance distortion algorithms crafted for real-time audio processing without delay.
- **Custom User Interface**: A fully custom JUCE LookAndFeel, PNG asset rendering, and a glowing aesthetic that definitely isn't copyrighted by the Tolkien estate.

## 🛠️ Installation

You can download the pre-compiled VST3 plugin directly from the [Releases](https://github.com/FenrirRhogar/OneKnobToRuleThemAll/releases) tab.

Alternatively, for an automatic installation on Windows into your VST3 host of choice:
1. Clone the repository.
2. Run the `install.bat` script located in the project root.
3. The script will automatically copy the pre-compiled VST3 file directly into your `C:\Program Files\Common Files\VST3` folder (requires admin privileges).

## 💻 Manual Build Instructions

OneKnobToRuleThemAll uses **CMake** and **JUCE 8**. You can compile the plugin yourself from the command line.

### Windows (Visual Studio)

```powershell
# 1. Generate project files (using your installed version of Visual Studio)
cmake -B build -G "Visual Studio 18 2026" -A x64

# 2. Build the plugin in Release mode using all CPU cores
cmake --build build --config Release --parallel
```

Once compiled, the plugin will be located at:
`build\OneKnobToRuleThemAll_artefacts\Release\VST3\OneKnobToRuleThemAll.vst3`

### macOS (Xcode)

```bash
# 1. Generate Xcode project files
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Release

# 2. Build the plugin
cmake --build build --config Release --parallel
```

Once compiled, the plugins will be located at:
- VST3: `build/OneKnobToRuleThemAll_artefacts/Release/VST3/OneKnobToRuleThemAll.vst3`
- AU: `build/OneKnobToRuleThemAll_artefacts/Release/AU/OneKnobToRuleThemAll.component`

## ☕ Support the Project

This plugin is completely free and open-source. However, if you use it in your mixes and find it helpful, please consider **[buying me a coffee on Gumroad!](https://fenrikos.gumroad.com/l/oneknobtorulethemall)** 

Your support helps keep this project alive and funds the development of future audio plugins!
