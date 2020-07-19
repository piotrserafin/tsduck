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
//  Representation of a DSM-CC UNM group_link_descriptor.
//
//----------------------------------------------------------------------------

#include "tsDSMCCUNMGroupLinkDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"DSMCC_UNM_group_link_descriptor"
#define MY_CLASS ts::DSMCCUNMGroupLinkDescriptor
#define MY_DID ts::DID_DSMCC_UNM_GROUP_LINK
#define MY_TID ts::TID_DSMCC_UNM
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::DSMCCUNMGroupLinkDescriptor::DSMCCUNMGroupLinkDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    position(0xFF),
    group_id(0xFFFFFFFF)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::DSMCCUNMGroupLinkDescriptor::DSMCCUNMGroupLinkDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    position(0xFF),
    group_id(0xFFFFFFFF)
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Clear
//----------------------------------------------------------------------------

void ts::DSMCCUNMGroupLinkDescriptor::clearContent()
{
    position = 0xFF;
    group_id = 0xFFFFFFFF;
}


//----------------------------------------------------------------------------
// Enumeration description of position.
//----------------------------------------------------------------------------

const ts::Enumeration ts::DSMCCUNMGroupLinkDescriptor::PositionEnum({
    {u"first",   ts::DSMCCUNMGroupLinkDescriptor::FIRST},
    {u"intermediate",   ts::DSMCCUNMGroupLinkDescriptor::INTERMEDIATE},
    {u"last",       ts::DSMCCUNMGroupLinkDescriptor::LAST},
    {u"undefined",  ts::DSMCCUNMGroupLinkDescriptor::UNDEFINED}
});


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCUNMGroupLinkDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(position);
    bbp->appendUInt32(group_id);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCUNMGroupLinkDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size == 5;

    if (_is_valid) {
        position = data[0];
        data++; size--;
        group_id = GetUInt32(data);
    }
    else {
        position = 0xFF;
        group_id = 0xFFFFFFFF;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCUNMGroupLinkDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const uint8_t position = payload[0];
        payload++; size--;
        const uint32_t group_id = GetUInt32(payload);
        
        strm << margin << UString::Format(u"Position: %d (%s)", {position, PositionEnum.name(position)}) << std::endl;
        strm << margin << UString::Format(u"GroupId: 0x%X ", {group_id}) << std::endl;
    }

    display.displayExtraData(payload, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCUNMGroupLinkDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setEnumAttribute(PositionEnum, u"position", position);
    root->setIntAttribute(u"group_id", group_id);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCUNMGroupLinkDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntEnumAttribute(position, PositionEnum, u"position", 0xFF) &&
           element->getIntAttribute(group_id, u"group_id", 0xFFFFFFFF);
}
