#pragma once
#include "framework.h"
#include "h3functions.hpp"
#include "LegacyH3ApiCompat.h"

constexpr UINT16 H3_MAX_OBJECTS = h3::limits::OBJECTS;

// Compatibility surface used by the object implementations carried over from
// the legacy RMG plugin. Registration and callbacks are provided by the current
// ObjectExtender API; these helpers only preserve the old source-level names.
#include "BaseClasses.h"
#include "ObjectExtenders/H3MapItemNew.h"
#include "ObjectExtenders/WaterObjects.h"
#include "ObjectExtenders/FlagsExtender.h"
#include "ObjectExtenders/TeamVisitFlags.h"

