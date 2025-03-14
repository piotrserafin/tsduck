//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPMACStreamLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"IPMAC_stream_location_descriptor"
#define MY_CLASS    ts::IPMACStreamLocationDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_INT_STREAM_LOC, ts::Standards::DVB, ts::TID_INT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::IPMACStreamLocationDescriptor::IPMACStreamLocationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::IPMACStreamLocationDescriptor::clearContent()
{
    network_id = 0;
    original_network_id = 0;
    transport_stream_id = 0;
    service_id = 0;
    component_tag = 0;
}

ts::IPMACStreamLocationDescriptor::IPMACStreamLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    IPMACStreamLocationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::IPMACStreamLocationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(network_id);
    buf.putUInt16(original_network_id);
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(service_id);
    buf.putUInt8(component_tag);
}

void ts::IPMACStreamLocationDescriptor::deserializePayload(PSIBuffer& buf)
{
    network_id = buf.getUInt16();
    original_network_id = buf.getUInt16();
    transport_stream_id = buf.getUInt16();
    service_id = buf.getUInt16();
    component_tag = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::IPMACStreamLocationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(9)) {
        disp << margin << UString::Format(u"Network id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Original network id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Transport stream id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Service id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Component tag: %n", buf.getUInt8()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::IPMACStreamLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"network_id", network_id, true);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"service_id", service_id, true);
    root->setIntAttribute(u"component_tag", component_tag, true);
}

bool ts::IPMACStreamLocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(network_id, u"network_id", true) &&
           element->getIntAttribute(original_network_id, u"original_network_id", true) &&
           element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
           element->getIntAttribute(service_id, u"service_id", true) &&
           element->getIntAttribute(component_tag, u"component_tag", true);
}
