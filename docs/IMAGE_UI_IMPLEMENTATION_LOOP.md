# Image UI Implementation Loop

This project is image-first. A user-supplied reference image is visual truth, not inspiration.

## Required loop for every UI change

1. **Lock the requested scope** — record the exact components, reference image paths, approved rectangles, and explicit non-targets before changing code or assets.
2. **Trace the actual asset path** — identify the source PNG, BinaryData identifier, C++ loading function, native image size, draw bounds, and click bounds. Do not assume an asset is active.
3. **Apply the smallest patch** — change only the traced asset or rendering path. Do not redesign adjacent controls or alter state logic unless the request explicitly includes it.
4. **Render evidence** — produce a native 1280x853 offscreen render for each requested state, plus crops for each changed rectangle.
5. **Compare against scope** — verify that every changed pixel is inside an approved rectangle. For image assets, verify native size equals draw bounds and that no runtime scaling, fallback asset, or duplicate draw path is active.
6. **Validate interaction separately** — run focused state tests for the requested behavior; rendering success does not prove state ownership, and state success does not prove the correct image was used.
7. **CI gate** — require compile/link, StateModelTests, Runtime Smoke Test, and bundle validation before replacing the candidate.
8. **Host gate** — mark the result as candidate only until the user verifies it in FL Studio. Capture the exact screenshot/objection and restart at step 2 if it differs.

## Non-negotiable image checks

- Every displayed state has one authoritative asset path.
- `image native size == draw bounds == click bounds` for image buttons/cells.
- No `drawImageWithin`, proportional fit, fractional placement, generated font substitute, or extra JUCE frame/text unless explicitly approved.
- The reference image may not remain visible beneath a dynamic state asset unless that overlap is explicitly part of the design.
- A request naming two UI defects authorizes only those defects; the commit file list is audited before CI.

## Required evidence in the final report

- exact source asset(s) used;
- the C++ rendering path replaced or verified;
- offscreen render for every requested state;
- focused test result and CI run;
- candidate SHA-256;
- explicit statement that FL Studio host confirmation is still pending, unless the user has completed it.
