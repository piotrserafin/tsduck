//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNetworkNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"network_name_descriptor"
#define MY_CLASS    ts::NetworkNameDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_NETWORK_NAME, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::NetworkNameDescriptor::NetworkNameDescriptor(const UString& name_) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    name(name_)
{
}

ts::NetworkNameDescriptor::NetworkNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    NetworkNameDescriptor()
{
    deserialize(duck, desc);
}

void ts::NetworkNameDescriptor::clearContent()
{
    name.clear();
}

ts::DescriptorDuplication ts::NetworkNameDescriptor::duplicationMode() const
{
    return DescriptorDuplication::REPLACE;
}


//----------------------------------------------------------------------------
// Binary serialization / deserialization.
//----------------------------------------------------------------------------

void ts::NetworkNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putString(name);
}

void ts::NetworkNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getString(name);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NetworkNameDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp << margin << "Name: \"" << buf.getString() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::NetworkNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"network_name", name);
}

bool ts::NetworkNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(name, u"network_name", true, u"", 0, MAX_DESCRIPTOR_SIZE - 2);
}
