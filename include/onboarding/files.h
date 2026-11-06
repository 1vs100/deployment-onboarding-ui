// The file side of the onboarding view: the manifest, per-file state, and the
// store that hashes and ingests files. Exported because a host serving the
// phone surface needs it to answer /status and accept uploads.
#pragma once

#include "core/catalog.h"
#include "core/file_store.h"
#include "core/manifest.h"
