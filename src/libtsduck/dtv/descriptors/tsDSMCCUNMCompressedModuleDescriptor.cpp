//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Piotr Serafin
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
//
//  Representation of a DSM-CC UNM compressed_module_descriptor.
//
//----------------------------------------------------------------------------

#include "tsDSMCCUNMCompressedModuleDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"DSMCC_UNM_compressed_module_descriptor"
#define MY_CLASS ts::DSMCCUNMCompressedModuleDescriptor
#define MY_DID ts::DID_DSMCC_UNM_COMPRESSED_MODULE
#define MY_TID ts::TID_DSMCC_UNM
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::DSMCCUNMCompressedModuleDescriptor::DSMCCUNMCompressedModuleDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    compression_method(0x00),
    original_size(0x00000000)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::DSMCCUNMCompressedModuleDescriptor::DSMCCUNMCompressedModuleDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    compression_method(0x00),
    original_size(0x00000000)
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Clear
//----------------------------------------------------------------------------

void ts::DSMCCUNMCompressedModuleDescriptor::clearContent()
{
    compression_method = 0x00;
    original_size = 0x00000000;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCUNMCompressedModuleDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(compression_method);
    bbp->appendUInt32(original_size);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCUNMCompressedModuleDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size == 5;

    if (_is_valid) {
        compression_method = data[0];
        data++; size--;
        original_size = GetUInt32(data);
    }
    else {
        compression_method = 0x00;
        original_size = 0x00000000;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCUNMCompressedModuleDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const uint8_t compression_method = payload[0];
        payload++; size--;
        const uint32_t original_size = GetUInt32(payload);
        
        strm << margin << UString::Format(u"Compression Method: %d (%s)", {compression_method}) << std::endl;
        strm << margin << UString::Format(u"Original Size: 0x%X ", {original_size}) << std::endl;
    }

    display.displayExtraData(payload, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCUNMCompressedModuleDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"compression_method", compression_method);
    root->setIntAttribute(u"original_size", original_size);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCUNMCompressedModuleDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(compression_method, u"compression_method", 0x00) &&
           element->getIntAttribute(original_size, u"original_size", 0x00000000);
}
