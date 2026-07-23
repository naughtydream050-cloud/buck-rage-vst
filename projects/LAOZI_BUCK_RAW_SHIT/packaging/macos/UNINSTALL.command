#!/bin/bash
set -euo pipefail

PLUGIN_NAME="LAOZI_BUCK_RAW_SHIT.vst3"
TARGET_PLUGIN="$HOME/Library/Audio/Plug-Ins/VST3/$PLUGIN_NAME"

echo "老子-BUCK RAW SHIT- Mac Uninstaller"
if [[ ! -e "$TARGET_PLUGIN" && ! -L "$TARGET_PLUGIN" ]]; then
  echo "対象のVST3はユーザー用フォルダにありません。"
  exit 0
fi
echo "削除対象: $TARGET_PLUGIN"
printf "このVST3だけを削除します。続行しますか？ [y/N]: "
IFS= read -r answer
case "$answer" in
  y|Y) rm -rf "$TARGET_PLUGIN"; echo "アンインストール完了。" ;;
  *) echo "キャンセルしました。" ;;
esac
if [[ -t 0 ]]; then read -n 1 -s -r -p "何かキーを押すと終了します。"; echo ""; fi
