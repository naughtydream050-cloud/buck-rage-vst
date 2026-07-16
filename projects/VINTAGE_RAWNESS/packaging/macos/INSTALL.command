#!/bin/bash
set -euo pipefail

PLUGIN_NAME="VINTAGE RAWNESS.vst3"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
SOURCE_PLUGIN="$SCRIPT_DIR/$PLUGIN_NAME"
SOURCE_BINARY="$SOURCE_PLUGIN/Contents/MacOS/VINTAGE RAWNESS"
TARGET_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
TARGET_PLUGIN="$TARGET_DIR/$PLUGIN_NAME"
TEMP_PLUGIN="$TARGET_DIR/.${PLUGIN_NAME}.installing.$$"
BACKUP_PLUGIN="$TARGET_DIR/.${PLUGIN_NAME}.backup.$$"

cleanup() {
  if [[ -e "$TEMP_PLUGIN" || -L "$TEMP_PLUGIN" ]]; then
    rm -rf "$TEMP_PLUGIN"
  fi
}

restore_backup() {
  if [[ ! -e "$TARGET_PLUGIN" && ! -L "$TARGET_PLUGIN" ]] &&
     [[ -e "$BACKUP_PLUGIN" || -L "$BACKUP_PLUGIN" ]]; then
    mv "$BACKUP_PLUGIN" "$TARGET_PLUGIN"
  fi
}

on_error() {
  local exit_code=$?
  echo ""
  echo "エラー: インストールに失敗しました（行: ${BASH_LINENO[0]}、終了コード: $exit_code）。"
  cleanup
  restore_backup
  exit "$exit_code"
}

trap on_error ERR
trap cleanup EXIT

echo "VINTAGE RAWNESS Mac Installer"
echo ""

if [[ ! -d "$SOURCE_PLUGIN" ]]; then
  echo "エラー: $PLUGIN_NAME がインストーラーと同じフォルダに見つかりません。"
  exit 1
fi

if [[ ! -f "$SOURCE_BINARY" || ! -s "$SOURCE_BINARY" ]]; then
  echo "エラー: VST3内部の実行ファイルが見つからないか、空です。"
  exit 1
fi

if ! codesign --verify --deep --strict "$SOURCE_PLUGIN"; then
  echo "エラー: 配布VST3のad-hoc署名を確認できません。ZIPを再取得してください。"
  exit 1
fi

mkdir -p "$TARGET_DIR"
cleanup
ditto "$SOURCE_PLUGIN" "$TEMP_PLUGIN"

if [[ ! -f "$TEMP_PLUGIN/Contents/MacOS/VINTAGE RAWNESS" ]]; then
  echo "エラー: コピー後のVST3内部実行ファイルを確認できません。"
  exit 1
fi

chmod +x "$TEMP_PLUGIN/Contents/MacOS/VINTAGE RAWNESS"
xattr -dr com.apple.quarantine "$TEMP_PLUGIN" 2>/dev/null || true

if ! codesign --verify --deep --strict "$TEMP_PLUGIN"; then
  echo "エラー: コピー後のcodesign検証に失敗しました。"
  exit 1
fi

if [[ -e "$TARGET_PLUGIN" || -L "$TARGET_PLUGIN" ]]; then
  echo "既存版を安全に置き換えます:"
  echo "$TARGET_PLUGIN"
  mv "$TARGET_PLUGIN" "$BACKUP_PLUGIN"
fi

mv "$TEMP_PLUGIN" "$TARGET_PLUGIN"

if [[ -e "$BACKUP_PLUGIN" || -L "$BACKUP_PLUGIN" ]]; then
  rm -rf "$BACKUP_PLUGIN"
fi

trap - ERR

echo ""
echo "インストール完了:"
echo "$TARGET_PLUGIN"
echo ""
echo "署名状態:"
codesign -dv --verbose=2 "$TARGET_PLUGIN" 2>&1 | grep -E "^(Identifier|Format|CodeDirectory|Signature)" || true
echo ""
echo "FL Studioを起動し、以下を実行してください。"
echo "Options → Manage plugins → Find installed plugins"
echo ""
echo "このVST3はテスト用ad-hoc署名です。Developer ID署名・notarizationは行っていません。"

if [[ -t 0 ]]; then
  echo ""
  read -n 1 -s -r -p "何かキーを押すと終了します。"
  echo ""
fi

