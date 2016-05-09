// This file is auto-generated by Current/scripts/full-test.sh.
// It is updated by running this script from the top-level directory,
// tests underneath which should be build in a bundle and run.
// For top-level tests, this file is useful to check in, so that non-*nix
// systems can too compile and run all the tests in a bundle.

#include "port.h"  // To have `std::{min/max}` work in Visual Studio, need port.h before STL headers.

#include "../Bricks/dflags/dflags.h"
#include "../3rdparty/gtest/gtest-main-with-dflags.h"

#define CURRENT_MOCK_TIME

#include "./dflags/test.cc"
#include "./dot/test.cc"
#include "./file/test.cc"
#include "./graph/test.cc"
#include "./net/http/headers/test.cc"
#include "./net/http/test.cc"
#include "./net/tcp/test.cc"
#include "./rtti/docu/docu_06rtti_01_test.cc"
#include "./rtti/test.cc"
#include "./strings/test.cc"
#include "./sync/test.cc"
#include "./template/test.cc"
#include "./time/test.cc"
#include "./util/test.cc"
#include "./waitable_atomic/test.cc"
