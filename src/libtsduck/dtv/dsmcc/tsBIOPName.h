//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of a BIOP::Name.
//
//!
//----------------------------------------------------------------------------

#pragma once

namespace ts {
    class TSDUCKDLL BIOPName {
    public:
        BIOPName() = default;

        ByteBlock _id_data {};
        ByteBlock _kind_data {};
    };
}  // namespace ts
