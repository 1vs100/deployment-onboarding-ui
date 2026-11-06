# A flavor selects the toolkit, not the product

Two UIs paint the same view: ImGui draw lists and plain SDL renderer
primitives. The build picks one with `ONBOARDING_UI_FLAVOR`, so a console build
links one toolkit and the other code is absent rather than dormant.

Both are worth keeping. ImGui brings widgets, a font atlas and a debugging story
that costs nothing here, but it is a third-party library in a signed console
package whose text input and threading assumptions are not ours. The sdl3 flavor
has no UI dependency at all beyond SDL, which is already linked, so a build that
must not carry ImGui still has a real UI.

The cost of two renderers is bounded by putting every decision that matters in
one place: layout produces a `Scene` of rectangles, text runs, bars and buttons,
and a flavor only rasterizes it. Anything that cannot be expressed in a `Scene`
is a design change, not a flavor change, and it goes in the shared layout so
both flavors move together.

Rejected: a runtime switch. It would link both toolkits into the package, which
is what the build-time option exists to avoid.
