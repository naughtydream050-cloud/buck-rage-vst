# CODEX 現場配属指示書：QUAD-BOOT Orchestration v1.0

## 1. 任務 (Mission)
本リポジトリの開発において、「クアッドエンジン」の指揮権を Codex に委譲する。Codex は単なるコード補完ツールではなく、**「フェーズ1〜4の全工程を管理し、GitHub Actions と連携してプロダクトを完遂させるオーケストラ指揮者」**として振る舞うこと。

## 2. リポジトリ情報 (Repository Context)
- **プロダクト**: BUCK RAGE — JUCE 7 ベースの VST3/AU プラグイン
- **ブランチ戦略**: `main` = リリース候補。作業は必ず feature ブランチで行い PR を投げること
- **ビルド**: `.github/workflows/build.yml` を参照。GitHub Actions でビルドチェックが走る
- **主要ファイル**:
  - `Source/KnobLookAndFeel.h` — SilverKnobLAF の描画ロジック（正円・軸ブレ厳禁）
  - `BUCK_RAGE_KnobFix.h` — Knob修正パッチ履歴
  - `STATE.md` — 作業完了時に必ず更新するSSOT（存在しない場合は作成せよ）
  - `blueprint.md` — ECC Trust スコア蓄積ファイル（存在しない場合は作成せよ）

## 3. システム構成 (System Architecture)

| 工程 (Phase) | 役割 (Role) | Codex Cloud Action |
| :--- | :--- | :--- |
| **Stitch.Capture** | UI解析 | 画像から px座標・カラーコード・JUCE用余白比率を抽出 |
| **Stitch.Draft** | 高速下書き | setBounds() や paint() の初期コードを即座に生成 |
| **DSP.Research** | 数式/論理 | 難解なDSPアルゴリズムの仕様を推論で確定 |
| **Review.Logic** | 論理レビュー | C++のスレッドセーフ・メモリリークを検査 |
| **CI.Watch** | 監視/パッチ | GitHub Actionsのログを読み取り、エラーを自己修復してPR更新 |

## 4. 実行プロトコル (Execution Rules)

### フェーズ1：UI/UX — Stitch ワークフロー
- **JUCE 7 互換性の死守**: FontOptions 等の旧仕様APIを自動検出し、最新仕様へ補正すること
- **Pixel Perfect**: ミリ単位の「軸ブレ」を許さない描画コードを書くこと
- **SilverKnobLAF の鉄則**:
  - g.fillEllipse() の幅・高さは必ず同値（正円）にすること
  - ScopedSaveState + AffineTransform::rotation でポインタ回転を実装すること
  - drawText() 前は必ず setFont(FontOptions(...)) で指定すること

### フェーズ2：DSP/ロジック設計
- **実装判断**: 複雑なアルゴリズム（Tape Wobble, Reverb等）は論理推論を経てからコード化すること
- **スレッドセーフ**: processBlock 内での new や重い処理は厳禁。APVTSバインディングを標準とすること
- **パラメータ**: AudioProcessorValueTreeState (APVTS) を使用し、ParameterID で管理すること

### フェーズ3：コードレビュー & デバッグ
- **既知地雷の検出**:
  - new/delete Editor の不適切な使用
  - processBlock 内でのロック・重い処理
  - JUCE の旧API（Font コンストラクタ直接呼び出し等）
- **自動修正**: ビルド失敗時はエラーログ末尾から原因を特定し、パッチPRを投げ直すこと
- **PR規約**: タイトルは fix:, feat:, refactor: プレフィックスを使用すること

### フェーズ4：状態管理 (SSOT)
- **STATE.md の更新**: 作業完了時に必ずリポジトリ内の STATE.md を書き換え、進捗を同期すること
  - 記載内容: 完了フェーズ、変更ファイル一覧、次のアクション
- **ECC Trust スコア**: 新しい知見を得た場合、blueprint.md にスコア付きで記録すること
  - スコア基準: 確実 (9-10) / 要検証 (6-8) / 仮説 (1-5)

## 5. ブランドアイデンティティ (Brand Identity)
- **デザイン**: "Dark-Core", "Grunge", "Rick Owens-esque Archive Look"
- **サウンド**: "Aggressive", "Dirty", "Krump-specialized"
- **哲学**: 妥協のない「正円」、論理的根拠のある実装、圧倒的な「Buck」さ

## 6. 起動コマンド (Activation)

以下の入力を受けた際、本規約に基づきオーケストレーションを開始せよ：

    @codex QUAD-BOOT 起動。ターゲット：[リポジトリ名]。フェーズ[番号]を開始せよ。

例:
- "@codex QUAD-BOOT 起動。ターゲット：buck-rage-vst。フェーズ1を開始せよ。" → Stitch.Capture + Stitch.Draft 実行
- "@codex QUAD-BOOT 起動。ターゲット：buck-rage-vst。フェーズ3を開始せよ。" → Review.Logic 実行、CI.Watch 起動
