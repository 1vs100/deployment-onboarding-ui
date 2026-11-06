# Design

The console surface and the phone surface are one product. Both are drawn on a
fixed 1280x720 canvas, and both flavors paint the same scene, so a flavor
changes the toolkit and not the design.

## Palette

Flat-neutral: neutral greys, one accent, no gradients, no brand colour. The
values live in `src/ui/design.h` and are repeated in the phone page's CSS.

| Token | Value | Use |
|---|---|---|
| background | `#101012` | page |
| surface | `#17171a` | panels |
| raised | `#1f1f23` | buttons, progress tracks |
| border | white at 8% | 1px outlines |
| text | `#f2f2f4` | body and titles |
| muted | `#9a9aa2` | labels, secondary lines |
| accent | `#4cc2ff` | focus ring, progress fill |
| success | `#6ccb5f` | verified |
| error | `#ff99a4` | rejected, invalid |

Green and red are semantic, not decoration.

## Type

Roboto Medium, embedded as bytes so nothing depends on a system font, which a
store app cannot reach anyway.

| Role | Size | Line box |
|---|---|---|
| title | 44 px | 61.6 px |
| body | 26 px | 36.4 px |
| small | 22 px | 30.8 px |

Type is rasterized at those pixel sizes and then scaled by the renderer's
logical presentation, the same way in both flavors.

## Grid

The canvas is 1280x720 with a 72 px margin on every side. The right column of
the get-files page starts at x=400 and is 808 px wide.

## Pages

1. **Checking** - title, one line about hashing, one panel with the file being
   hashed and the overall progress.
2. **Get files** - QR code and the address on the left, the same address in
   text, the counts, the missing files and the two pickers on the right. While
   something arrives, the right column shows progress instead of the counts.
3. **Ready** - one sentence and one button.
4. **Done** - one sentence, no controls.

## Controls

One focus ring, moved by the D-pad, the left stick, the arrow keys, Tab, or the
mouse. Every page is usable with a controller alone. A disabled button keeps its
place so the page still says what to do and why it cannot be done yet.

## Parity rule

If a change cannot be expressed in a `Scene`, it does not belong in a flavor.
Both flavors draw from the same scene, so the two screenshots of one page differ
only where the rasterizers genuinely differ.
