//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// angle_version_info.cpp: ANGLE version queries.

#include "common/angle_version.h"

namespace angle
{
int GetANGLERevision()
{
    return ANGLE_REVISION;
}

const char *GetANGLEVersionString()
{
    return ANGLE_VERSION_STRING;
}

const char *GetANGLECommitHash()
{
    return ANGLE_COMMIT_HASH;
}

int GetANGLECommitHashSize()
{
    return ANGLE_COMMIT_HASH_SIZE;
}

const char *GetANGLEUnknownHash()
{
    return ANGLE_COMMIT_HASH_UNKNOWN;
}
}  // namespace angle
