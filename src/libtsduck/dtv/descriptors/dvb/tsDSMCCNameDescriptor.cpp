//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dsmcc_name_descriptor"
#define MY_CLASS    ts::DSMCCNameDescriptor
#define MY_DID      ts::DID_DSMCC_NAME
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DSMCCNameDescriptor::DSMCCNameDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DSMCCNameDescriptor::DSMCCNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    name()
{
    deserialize(duck, desc);
}

void ts::DSMCCNameDescriptor::clearContent()
{
    name.clear();
}


//----------------------------------------------------------------------------
// Binary serialization
//----------------------------------------------------------------------------

void ts::DSMCCNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putString(name);
}

void ts::DSMCCNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getString(name);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCNameDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    disp << margin << "Module or Group Name: \"" << buf.getString() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::DSMCCNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"name", name);
}

bool ts::DSMCCNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(name, u"name", true, u"", 0, MAX_DESCRIPTOR_SIZE - 2);
}