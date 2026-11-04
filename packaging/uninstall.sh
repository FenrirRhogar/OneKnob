#!/usr/bin/env bash
# Removes One Knob as installed by install.sh. Pass --system for a system-wide install.
set -euo pipefail

vst3_dir="$HOME/.vst3"
lv2_dir="$HOME/.lv2"
sudo=""

if [[ "${1:-}" == "--system" ]]; then
    vst3_dir="/usr/lib/vst3"
    lv2_dir="/usr/lib/lv2"
    sudo="sudo"
fi

echo "Removing One Knob:"
for bundle in "$vst3_dir/One Knob.vst3" "$lv2_dir/One Knob.lv2"; do
    [[ -e "$bundle" ]] || continue
    $sudo rm -rf "$bundle"
    echo "  $bundle"
done
echo "Done."
