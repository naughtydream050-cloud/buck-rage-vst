#!/bin/bash
set -euo pipefail

PLUGIN_NAME="LAOZI_BUCK_RAW_SHIT.vst3"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
SOURCE_PLUGIN="$SCRIPT_DIR/$PLUGIN_NAME"
SOURCE_BINARY="$SOURCE_PLUGIN/Contents/MacOS/LAOZI_BUCK_RAW_SHIT"
TARGET_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
TARGET_PLUGIN="$TARGET_DIR/$PLUGIN_NAME"
TEMP_PLUGIN="$TARGET_DIR/.${PLUGIN_NAME}.installing.$$"
BACKUP_PLUGIN="$TARGET_DIR/.${PLUGIN_NAME}.backup.$$"

cleanup() { [[ -e "$TEMP_PLUGIN" || -L "$TEMP_PLUGIN" ]] && rm -rf "$TEMP_PLUGIN"; }
restore_backup() {
  if [[ ! -e "$TARGET_PLUGIN" && ! -L "$TARGET_PLUGIN" ]] && [[ -e "$BACKUP_PLUGIN" || -L "$BACKUP_PLUGIN" ]]; then
    mv "$BACKUP_PLUGIN" "$TARGET_PLUGIN"
  fi
}
on_error() { local code=$?; echo "エラー: インストールに失敗しました（終了コード: $code）。"; cleanup; restore_backup; exit "$code"; }
trap on_error ERR
trap cleanup EXIT

echo "老子-BUCK RAW SHIT- Mac Installer"
if [[ ! -d "$SOURCE_PLUGIN" || ! -s "$SOURCE_BINARY" ]]; then
  echo "エラー: 配布パック内のVST3 bundleまたは実行ファイルが見つかりません。"
  exit 1
fi
if ! codesign --verify --deep --strict "$SOURCE_PLUGIN"; then
  echo "エラー: 配布VST3のad-hoc署名検証に失敗しました。ZIPを再取得してください。"
  exit 1
fi

mkdir -p "$TARGET_DIR"
cleanup
ditto "$SOURCE_PLUGIN" "$TEMP_PLUGIN"
chmod +x "$TEMP_PLUGIN/Contents/MacOS/LAOZI_BUCK_RAW_SHIT"
xattr -dr com.apple.quarantine "$TEMP_PLUGIN" 2>/dev/null || true
if ! codesign --verify --deep --strict "$TEMP_PLUGIN"; then
  echo "エラー: コピー後のcodesign検証に失敗しました。"
  exit 1
fi
if [[ -e "$TARGET_PLUGIN" || -L "$TARGET_PLUGIN" ]]; then
  echo "既存版を安全に置換します: $TARGET_PLUGIN"
  mv "$TARGET_PLUGIN" "$BACKUP_PLUGIN"
fi
mv "$TEMP_PLUGIN" "$TARGET_PLUGIN"
[[ -e "$BACKUP_PLUGIN" || -L "$BACKUP_PLUGIN" ]] && rm -rf "$BACKUP_PLUGIN"
trap - ERR

echo ""
echo "インストール完了:"
echo "$TARGET_PLUGIN"
echo ""
echo "FL Studioを起動し、Options → Manage plugins → Find installed plugins を実行してください。"
echo "このテスト版はad-hoc署名です。Developer ID署名・notarizationは未実施です。"
if [[ -t 0 ]]; then read -n 1 -s -r -p "何かキーを押すと終了します。"; echo ""; fi
