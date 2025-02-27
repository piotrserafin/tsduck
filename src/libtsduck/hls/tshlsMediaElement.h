//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Description of a media "element" inside an HLS playlist.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsURL.h"
#include "tsStringifyInterface.h"

namespace ts::hls {
    //!
    //! Description of a media "element" (sub-playlist or segment) inside an HLS playlist.
    //! @ingroup libtsduck hls
    //!
    class TSDUCKDLL MediaElement: public StringifyInterface
    {
    public:
        //!
        //! Constructor.
        //!
        MediaElement() = default;

        //!
        //! Get the URL string to use.
        //! @return The URL string to use.
        //!
        UString urlString() const;

        // Implementation of StringifyInterface
        virtual UString toString() const override;

        // Public fields.
        UString relative_uri {};  //!< Relative URI, verbatime from playlist.
        UString file_path {};     //!< Full file path.
        URL     url {};           //!< Full URL, invalid if accessed by file path only.
    };
}
