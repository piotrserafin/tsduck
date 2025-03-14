//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSNGDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DSNG_descriptor"
#define MY_CLASS    ts::DSNGDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_DSNG, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DSNGDescriptor::DSNGDescriptor(const UString& id) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    station_identification(id)
{
}

void ts::DSNGDescriptor::clearContent()
{
    station_identification.clear();
}

ts::DSNGDescriptor::DSNGDescriptor(DuckContext& duck, const Descriptor& desc) :
    DSNGDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSNGDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putString(station_identification);
}

void ts::DSNGDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getString(station_identification);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSNGDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp << margin << "Station identification: \"" << buf.getString() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSNGDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"station_identification", station_identification);
}

bool ts::DSNGDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(station_identification, u"station_identification", true, u"", 0, MAX_DESCRIPTOR_SIZE - 2);
}
