#!/bin/bash
set -euo pipefail

PLUGIN_NAME="VINTAGE RAWNESS.vst3"
TARGET_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
TARGET_PLUGIN="$TARGET_DIR/$PLUGIN_NAME"
EXPECTED_TARGET="$HOME/Library/Audio/Plug-Ins/VST3/VINTAGE RAWNESS.vst3"

echo "VINTAGE RAWNESS Mac Uninstaller"
echo ""

if [[ "$TARGET_PLUGIN" != "$EXPECTED_TARGET" ]]; then
  echo "エラー: 削除対象パスが安全条件と一致しません。"
  exit 1
fi

if [[ ! -e "$TARGET_PLUGIN" && ! -L "$TARGET_PLUGIN" ]]; then
  echo "VINTAGE RAWNESSはユーザー用VST3フォルダにインストールされていません。"
  exit 0
fi

echo "次のVINTAGE RAWNESSだけを削除します:"
echo "$TARGET_PLUGIN"
echo ""
printf "削除してよろしいですか？ [y/N]: "
IFS= read -r answer

case "$answer" in
  y|Y)
    if [[ -L "$TARGET_PLUGIN" ]]; then
      rm -f "$TARGET_PLUGIN"
    else
      rm -rf "$TARGET_PLUGIN"
    fi
    echo ""
    echo "アンインストール完了。ほかのプラグインは変更していません。"
    ;;
  *)
    echo ""
    echo "キャンセルしました。"
    ;;
esac

if [[ -t 0 ]]; then
  echo ""
  read -n 1 -s -r -p "何かキーを押すと終了します。"
  echo ""
fi

