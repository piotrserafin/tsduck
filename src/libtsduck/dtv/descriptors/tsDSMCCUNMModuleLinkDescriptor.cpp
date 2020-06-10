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
//  Representation of a DSM-CC UNM module_link_descriptor.
//
//----------------------------------------------------------------------------

#include "tsDSMCCUNMModuleLinkDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"DSMCC_UNM_module_link_descriptor"
#define MY_CLASS ts::DSMCCUNMModuleLinkDescriptor
#define MY_DID ts::DID_DSMCC_UNM_MODULE_LINK
#define MY_TID ts::TID_DSMCC_UNM
#define MY_STD ts::STD_DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::DSMCCUNMModuleLinkDescriptor::DSMCCUNMModuleLinkDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    position(0xFF),
    module_id(0xFFFF)
{
    _is_valid = true;
}

//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::DSMCCUNMModuleLinkDescriptor::DSMCCUNMModuleLinkDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    position(0xFF),
    module_id(0xFFFF)
{
    deserialize(duck, desc);
}

//----------------------------------------------------------------------------
// Enumeration description of position.
//----------------------------------------------------------------------------

const ts::Enumeration ts::DSMCCUNMModuleLinkDescriptor::PositionEnum({
    {u"first",   ts::DSMCCUNMModuleLinkDescriptor::FIRST},
    {u"intermediate",   ts::DSMCCUNMModuleLinkDescriptor::INTERMEDIATE},
    {u"last",       ts::DSMCCUNMModuleLinkDescriptor::LAST},
    {u"undefined",  ts::DSMCCUNMModuleLinkDescriptor::UNDEFINED}
});

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCUNMModuleLinkDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(position);
    bbp->appendUInt16(module_id);
    serializeEnd(desc, bbp);
}

//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCUNMModuleLinkDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size == 3;

    if (_is_valid) {
        position = data[0];
        data++; size--;
        module_id = GetUInt16(data);
    }
    else {
        position = 0xFF;
        module_id = 0xFFFF;
    }
}

//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCUNMModuleLinkDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const uint8_t position = payload[0];
        payload++; size--;
        const uint16_t module_id = GetUInt16(payload);
        
        strm << margin << UString::Format(u"Position: %d (%s)", {position, PositionEnum.name(position)}) << std::endl;
        strm << margin << UString::Format(u"ModuleId: 0x%X ", {module_id}) << std::endl;
    }

    display.displayExtraData(payload, size, indent);
}

//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCUNMModuleLinkDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setEnumAttribute(PositionEnum, u"position", position);
    root->setIntAttribute(u"module_id", module_id);
}

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::DSMCCUNMModuleLinkDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntEnumAttribute(position, PositionEnum, u"position", 0xFF) &&
        element->getIntAttribute(module_id, u"module_id", 0xFFFF);
}
