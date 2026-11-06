# The view owns the files it reports on

This repository holds more than drawing code: the manifest, the per-file state,
the hash store and the phone page all live here, while the game keeps only a call
to `EnsureGameFiles` and its own pickers and sockets.

A view that draws progress for files somebody else is fetching has to be told
what the state is, in a second contract that can disagree with the label it is
drawing. Keeping the store here means the page that says "9 of 11 files ready"
and the code that counts them are the same code, and it gives this repository
something to render in its own demo without a game, a console or a package. The
`Platform` struct is the whole boundary: pickers, worker threads, LAN addresses
and the socket listener, because those are the parts that need platform APIs.

Rejected: extracting the drawing code alone. It would have needed a second
abstraction for file state, and the phone page, which is the surface players
actually touch, could not have moved with the screens that advertise its URL.
