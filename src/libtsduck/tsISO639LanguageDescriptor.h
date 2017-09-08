//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISO_639_language_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISO_639_language_descriptor
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.18.
    //!
    class TSDUCKDLL ISO639LanguageDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Language entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            std::string language_code;  //!< ISO-639 language code, 3 characters.
            uint8_t     audio_type;     //!< Audio type.

            //!
            //! Default constructor.
            //! @param [in] code ISO-639 language code, 3 characters, as a C-string. Can be null.
            //! @param [in] type Audio type.
            //!
            Entry(const char* code = 0, uint8_t type = 0) :
                language_code(code != 0 ? code : ""),
                audio_type(type)
            {
            }

            //!
            //! Constructor.
            //! @param [in] code ISO-639 language code, 3 characters, as a C++ string.
            //! @param [in] type Audio type.
            //!
            Entry(const std::string& code, uint8_t type) :
                language_code(code),
                audio_type(type)
            {
            }
        };

        //!
        //! List of language entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of language entries to fit in 255 bytes.
        //!
        static const size_t MAX_ENTRIES = 63;

        // ISO639LanguageDescriptor public members:
        EntryList entries;  //!< List of language entries.

        //!
        //! Default constructor.
        //!
        ISO639LanguageDescriptor();

        //!
        //! Constructor with one language code.
        //! @param [in] code ISO-639 language code, 3 characters.
        //! @param [in] type Audio type.
        //!
        ISO639LanguageDescriptor(const std::string& code, uint8_t type);

        //!
        //! Constructor from a binary descriptor
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISO639LanguageDescriptor(const Descriptor& bin);

        // Inherited methods
        virtual void serialize(Descriptor&) const;
        virtual void deserialize(const Descriptor&);
        virtual XML::Element* toXML(XML& xml, XML::Element* parent) const;
        virtual void fromXML(XML& xml, const XML::Element* element);

        //!
        //! Static method to display a descriptor.
        //! @param [in,out] display Display engine.
        //! @param [in] did Descriptor id.
        //! @param [in] payload Address of the descriptor payload.
        //! @param [in] size Size in bytes of the descriptor payload.
        //! @param [in] indent Indentation width.
        //! @param [in] tid Table id of table containing the descriptors.
        //! @param [in] pds Private Data Specifier. Used to interpret private descriptors.
        //!
        static void DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds);
    };
}
