//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCBIOPMessage.h"
#include "tsNames.h"
#include "tsxmlElement.h"
#include <cstring>


namespace {
    // BIOP wire format pads kind/id byte fields with trailing NULs. Return the
    // length without those trailing zeros; callers then build a string of their
    // chosen type from data() up to that length.
    size_t TrimmedSize(const ts::ByteBlock& bytes)
    {
        size_t len = bytes.size();
        while (len > 0 && bytes[len - 1] == 0) {
            --len;
        }
        return len;
    }

    // Build a NUL-trimmed std::string from a BIOP kind/id byte block.
    std::string KindFromBytes(const ts::ByteBlock& bytes)
    {
        return std::string(reinterpret_cast<const char*>(bytes.data()), TrimmedSize(bytes));
    }
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Check if the header is valid
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::isValid() const
{
    if (magic != BIOP_MAGIC) {
        return false;
    }

    if (version_major != BIOP_VERSION_MAJOR) {
        return false;
    }

    if (byte_order != BIOP_BYTE_ORDER_BIG_ENDIAN) {
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Clear the content
//----------------------------------------------------------------------------

void ts::BIOPMessageHeader::clear()
{
    magic = BIOP_MAGIC;
    version_major = BIOP_VERSION_MAJOR;
    version_minor = BIOP_VERSION_MINOR;
    byte_order = BIOP_BYTE_ORDER_BIG_ENDIAN;
    message_type = BIOP_MESSAGE_TYPE_STANDARD;
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Serialize
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::serialize(PSIBuffer& buf) const
{
    if (buf.remainingWriteBytes() < HEADER_SIZE) {
        buf.setUserError();
        return false;
    }

    buf.putUInt32(magic);
    buf.putUInt8(version_major);
    buf.putUInt8(version_minor);
    buf.putUInt8(byte_order);
    buf.putUInt8(message_type);

    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Deserialize
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::deserialize(PSIBuffer& buf)
{
    if (buf.remainingReadBytes() < HEADER_SIZE) {
        buf.setUserError();
        return false;
    }

    if (!buf.isBigEndian()) {
        buf.setBigEndian();
    }

    magic = buf.getUInt32();
    version_major = buf.getUInt8();
    version_minor = buf.getUInt8();
    byte_order = buf.getUInt8();
    message_type = buf.getUInt8();

    if (!isValid()) {
        buf.setUserError();
        return false;
    }

    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Display
//----------------------------------------------------------------------------

void ts::BIOPMessageHeader::display(TablesDisplay& disp, const UString& margin) const
{
    disp << margin << "BIOP Message Header:" << std::endl;
    disp << margin << UString::Format(u"  Magic: %n", magic);

    if (magic == BIOP_MAGIC) {
        disp << " (ASCII: \"BIOP\")";
    }
    else {
        disp << " (Invalid magic number)";
    }
    disp << std::endl;

    disp << margin << UString::Format(u"  BIOP version: %d.%d", version_major, version_minor) << std::endl;

    disp << margin << "  Byte order: ";
    if (byte_order == BIOP_BYTE_ORDER_BIG_ENDIAN) {
        disp << "Big-endian";
    }
    else {
        disp << UString::Format(u"Invalid (%n)", byte_order);
    }
    disp << std::endl;

    disp << margin << UString::Format(u"  Message type: %n", message_type) << std::endl;
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Static Display
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    if (buf.remainingReadBytes() < HEADER_SIZE) {
        disp.displayExtraData(buf, margin);
        return false;
    }

    BIOPMessageHeader header;
    if (header.deserialize(buf)) {
        header.display(disp, margin);
        return true;
    }
    else {
        disp << margin << "Invalid BIOP message header" << std::endl;
        disp.displayExtraData(buf, margin);
        return false;
    }
}


//----------------------------------------------------------------------------
// BIOPMessage - kindTag (strip trailing nulls)
//----------------------------------------------------------------------------

std::string ts::BIOPMessage::kindTag() const
{
    return KindFromBytes(object_kind);
}


//----------------------------------------------------------------------------
// BIOPMessage - CreateForKind factory
//----------------------------------------------------------------------------

std::unique_ptr<ts::BIOPMessage> ts::BIOPMessage::CreateForKind(const std::string& tag)
{
    if (tag == BIOPObjectKind::FILE) {
        return std::make_unique<BIOPFileMessage>();
    }
    if (tag == BIOPObjectKind::SERVICE_GATEWAY || tag == BIOPObjectKind::DIRECTORY) {
        return std::make_unique<BIOPBindingListMessage>();
    }
    if (tag == BIOPObjectKind::STREAM) {
        return std::make_unique<BIOPStreamMessage>();
    }
    if (tag == BIOPObjectKind::STREAM_EVENT) {
        return std::make_unique<BIOPStreamEventMessage>();
    }
    return nullptr;
}


//----------------------------------------------------------------------------
// BIOPMessage - static Parse factory
//----------------------------------------------------------------------------

std::unique_ptr<ts::BIOPMessage> ts::BIOPMessage::Parse(PSIBuffer& buf)
{
    BIOPMessageHeader header;
    if (!header.deserialize(buf)) {
        return nullptr;
    }

    buf.pushReadSizeFromLength(32);  // message_size

    ByteBlock key, kind;
    buf.getBytes(key, buf.getUInt8());
    buf.getBytes(kind, buf.getUInt32());

    auto msg = CreateForKind(KindFromBytes(kind));
    bool ok = false;
    if (msg) {
        msg->header = std::move(header);
        msg->object_key = std::move(key);
        msg->object_kind = std::move(kind);
        buf.getBytes(msg->object_info, buf.getUInt16());

        msg->service_contexts.resize(buf.getUInt8());
        for (auto& ctx : msg->service_contexts) {
            ctx.deserialize(buf);
        }

        buf.pushReadSizeFromLength(32);  // messageBody_length
        ok = msg->deserializeBody(buf) && !buf.error();
        buf.popState();
    }
    buf.popState();

    return ok ? std::move(msg) : nullptr;
}

//----------------------------------------------------------------------------
// BIOPFileMessage - deserializeBody
//----------------------------------------------------------------------------

bool ts::BIOPFileMessage::deserializeBody(PSIBuffer& buf)
{
    const uint32_t content_length = buf.getUInt32();
    buf.getBytes(content, content_length);
    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPNameComponent helpers
//----------------------------------------------------------------------------

ts::UString ts::BIOPNameComponent::idString() const
{
    return UString::FromUTF8(reinterpret_cast<const char*>(id.data()), TrimmedSize(id));
}


std::string ts::BIOPNameComponent::kindTag() const
{
    return std::string(reinterpret_cast<const char*>(kind.data()), TrimmedSize(kind));
}


//----------------------------------------------------------------------------
// BIOPBinding - deserialize one binding
//----------------------------------------------------------------------------

bool ts::BIOPBinding::deserialize(PSIBuffer& buf)
{
    const uint8_t name_comp_count = buf.getUInt8();
    name.clear();
    name.reserve(name_comp_count);
    for (uint8_t i = 0; i < name_comp_count; ++i) {
        BIOPNameComponent name_comp;
        const uint8_t id_len = buf.getUInt8();
        buf.getBytes(name_comp.id, id_len);
        const uint8_t kind_len = buf.getUInt8();
        buf.getBytes(name_comp.kind, kind_len);
        name.push_back(std::move(name_comp));
    }

    binding_type = buf.getUInt8();
    ior.deserialize(buf);

    const uint16_t object_info_len = buf.getUInt16();
    buf.getBytes(object_info, object_info_len);

    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPBinding - target IOR location and joined path helpers
//----------------------------------------------------------------------------

std::optional<std::pair<uint16_t, ts::ByteBlock>> ts::BIOPBinding::targetLocation() const
{
    for (const auto& profile : ior.tagged_profiles) {
        if (profile.profile_id_tag != DSMCC_TAG_BIOP) {
            continue;
        }
        for (const auto& lite_comp : profile.lite_components) {
            if (lite_comp.component_id_tag == DSMCC_TAG_OBJECT_LOCATION) {
                return std::make_pair(lite_comp.module_id, lite_comp.object_key_data);
            }
        }
    }
    return std::nullopt;
}


ts::UString ts::BIOPBinding::pathString() const
{
    UString out;
    for (const auto& nc : name) {
        if (!out.empty()) {
            out += u"/";
        }
        out += nc.idString();
    }
    return out;
}


//----------------------------------------------------------------------------
// BIOPBindingListMessage - deserializeBody (shared by Directory / ServiceGateway)
//----------------------------------------------------------------------------

bool ts::BIOPBindingListMessage::deserializeBody(PSIBuffer& buf)
{
    const uint16_t count = buf.getUInt16();
    bindings.clear();
    bindings.reserve(count);
    for (uint16_t i = 0; i < count; ++i) {
        BIOPBinding b;
        if (!b.deserialize(buf)) {
            return false;
        }
        bindings.push_back(std::move(b));
    }
    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - toXML / fromXML
//----------------------------------------------------------------------------

void ts::BIOPMessageHeader::toXML(DuckContext& duck, xml::Element* element) const
{
    element->setIntAttribute(u"magic", magic, true);
    element->setIntAttribute(u"version_major", version_major, true);
    element->setIntAttribute(u"version_minor", version_minor, true);
    element->setIntAttribute(u"byte_order", byte_order, true);
    element->setIntAttribute(u"message_type", message_type, true);
}

bool ts::BIOPMessageHeader::fromXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(magic, u"magic", false, BIOP_MAGIC) &&
           element->getIntAttribute(version_major, u"version_major", false, BIOP_VERSION_MAJOR) &&
           element->getIntAttribute(version_minor, u"version_minor", false, BIOP_VERSION_MINOR) &&
           element->getIntAttribute(byte_order, u"byte_order", false, BIOP_BYTE_ORDER_BIG_ENDIAN) &&
           element->getIntAttribute(message_type, u"message_type", false, BIOP_MESSAGE_TYPE_STANDARD);
}


//----------------------------------------------------------------------------
// BIOPServiceContext - deserialize
//----------------------------------------------------------------------------

bool ts::BIOPServiceContext::deserialize(PSIBuffer& buf)
{
    context_id = buf.getUInt32();
    buf.getBytes(context_data, buf.getUInt16());
    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPServiceContext - toXML / fromXML
//----------------------------------------------------------------------------

void ts::BIOPServiceContext::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"service_context");
    e->setIntAttribute(u"context_id", context_id, true);
    e->addHexaTextChild(u"context_data", context_data, true);
}

bool ts::BIOPServiceContext::fromXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(context_id, u"context_id", true) &&
           element->getHexaTextChild(context_data, u"context_data");
}


//----------------------------------------------------------------------------
// BIOPNameComponent - toXML / fromXML
//----------------------------------------------------------------------------

void ts::BIOPNameComponent::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"name_component");
    e->addHexaTextChild(u"id", id, true);
    e->addHexaTextChild(u"kind", kind, true);
}

bool ts::BIOPNameComponent::fromXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaTextChild(id, u"id") &&
           element->getHexaTextChild(kind, u"kind");
}


//----------------------------------------------------------------------------
// BIOPBinding - toXML / fromXML
//----------------------------------------------------------------------------

void ts::BIOPBinding::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"binding");
    e->setIntAttribute(u"binding_type", binding_type, true);
    for (const auto& nc : name) {
        nc.toXML(duck, e);
    }
    ior.toXML(duck, e);
    e->addHexaTextChild(u"object_info", object_info, true);
}

bool ts::BIOPBinding::fromXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getIntAttribute(binding_type, u"binding_type", true);

    for (auto& xnc : element->children(u"name_component", &ok)) {
        ok = ok && name.emplace_back().fromXML(duck, &xnc);
    }

    const xml::Element* xior = element->findFirstChild(u"IOR", true);
    ok = ok && xior != nullptr && ior.fromXML(duck, xior);

    ByteBlock oi;
    if (ok && element->getHexaTextChild(oi, u"object_info", false)) {
        object_info = std::move(oi);
    }

    return ok;
}


//----------------------------------------------------------------------------
// BIOPMessage - toXML / fromXML (common fields + body dispatch)
//----------------------------------------------------------------------------

void ts::BIOPMessage::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"BIOP_message");
    e->setAttribute(u"object_kind", UString::FromUTF8(kindTag()));
    header.toXML(duck, e);
    e->addHexaTextChild(u"object_key", object_key, true);
    e->addHexaTextChild(u"object_info", object_info, true);
    for (const auto& ctx : service_contexts) {
        ctx.toXML(duck, e);
    }
    toXMLBody(duck, e);
}

bool ts::BIOPMessage::fromXML(DuckContext& duck, const xml::Element* element)
{
    UString kind_str;
    bool ok = element->getAttribute(kind_str, u"object_kind", true);
    if (ok) {
        // Rebuild the on-wire form: trimmed tag bytes followed by a single NUL terminator.
        const std::string tag = kind_str.toUTF8();
        object_kind.assign(tag.begin(), tag.end());
        object_kind.push_back(0);
    }

    ok = ok && header.fromXML(duck, element);

    ok = ok && element->getHexaTextChild(object_key, u"object_key");

    ByteBlock oi;
    if (ok && element->getHexaTextChild(oi, u"object_info", false)) {
        object_info = std::move(oi);
    }

    for (auto& xctx : element->children(u"service_context", &ok)) {
        ok = ok && service_contexts.emplace_back().fromXML(duck, &xctx);
    }

    ok = ok && fromXMLBody(duck, element);
    return ok;
}


//----------------------------------------------------------------------------
// BIOPMessage - FromXML static factory
//----------------------------------------------------------------------------

std::unique_ptr<ts::BIOPMessage> ts::BIOPMessage::FromXML(DuckContext& duck, const xml::Element* element)
{
    std::unique_ptr<ts::BIOPMessage> msg;
    UString kind_str;
    if (element->getAttribute(kind_str, u"object_kind", true)) {
        msg = CreateForKind(kind_str.toUTF8());
        if (!msg || !msg->fromXML(duck, element)) {
            msg.reset();
        }
    }
    return msg;
}


//----------------------------------------------------------------------------
// BIOPFileMessage - toXMLBody / fromXMLBody
//----------------------------------------------------------------------------

void ts::BIOPFileMessage::toXMLBody(DuckContext& duck, xml::Element* msg_element) const
{
    xml::Element* body = msg_element->addElement(u"fil");
    body->addHexaTextChild(u"content", content, true);
}

bool ts::BIOPFileMessage::fromXMLBody(DuckContext& duck, const xml::Element* msg_element)
{
    const xml::Element* body = msg_element->findFirstChild(u"fil", true);
    return body != nullptr && body->getHexaTextChild(content, u"content");
}


//----------------------------------------------------------------------------
// BIOPBindingListMessage - toXMLBody / fromXMLBody
//----------------------------------------------------------------------------

void ts::BIOPBindingListMessage::toXMLBody(DuckContext& duck, xml::Element* msg_element) const
{
    xml::Element* body = msg_element->addElement(u"bindings");
    for (const auto& b : bindings) {
        b.toXML(duck, body);
    }
}

bool ts::BIOPBindingListMessage::fromXMLBody(DuckContext& duck, const xml::Element* msg_element)
{
    const xml::Element* body = msg_element->findFirstChild(u"bindings", true);
    if (body == nullptr) {
        return false;
    }
    bool ok = true;
    for (auto& xb : body->children(u"binding", &ok)) {
        ok = ok && bindings.emplace_back().fromXML(duck, &xb);
    }
    return ok;
}


//----------------------------------------------------------------------------
// BIOPStreamMessage - deserializeBody
//----------------------------------------------------------------------------

bool ts::BIOPStreamMessage::deserializeBody(PSIBuffer& buf)
{
    // aDescription: info_length (uint32) + DSM::Stream::Info (4 bytes)
    const uint32_t info_length = buf.getUInt32();
    if (info_length >= 4) {
        info.audio_count = buf.getUInt8();
        info.video_count = buf.getUInt8();
        info.data_count = buf.getUInt8();
        info.event_count = buf.getUInt8();
        // Skip any remaining bytes in aDescription
        if (info_length > 4) {
            buf.skipBytes(info_length - 4);
        }
    }
    else {
        buf.skipBytes(info_length);
    }

    // Taps
    const uint16_t taps_count = buf.getUInt16();
    taps.resize(taps_count);
    for (auto& tap : taps) {
        tap.deserialize(buf);
    }
    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPStreamMessage - toXMLBody / fromXMLBody
//----------------------------------------------------------------------------

void ts::BIOPStreamMessage::toXMLBody(DuckContext& duck, xml::Element* msg_element) const
{
    xml::Element* body = msg_element->addElement(u"stream_body");
    xml::Element* xi = body->addElement(u"stream_info");
    xi->setIntAttribute(u"audio", info.audio_count);
    xi->setIntAttribute(u"video", info.video_count);
    xi->setIntAttribute(u"data", info.data_count);
    xi->setIntAttribute(u"event", info.event_count);
    for (const auto& tap : taps) {
        tap.toXML(duck, body);
    }
}

bool ts::BIOPStreamMessage::fromXMLBody(DuckContext& duck, const xml::Element* msg_element)
{
    const xml::Element* body = msg_element->findFirstChild(u"stream_body", true);
    if (body == nullptr) {
        return false;
    }
    const xml::Element* xi = body->findFirstChild(u"stream_info", true);
    if (xi == nullptr) {
        return false;
    }
    bool ok = xi->getIntAttribute(info.audio_count, u"audio", false, 0) &&
              xi->getIntAttribute(info.video_count, u"video", false, 0) &&
              xi->getIntAttribute(info.data_count, u"data", false, 0) &&
              xi->getIntAttribute(info.event_count, u"event", false, 0);
    for (auto& xt : body->children(u"Tap", &ok)) {
        ok = ok && taps.emplace_back().fromXML(duck, &xt, nullptr);
    }
    return ok;
}


//----------------------------------------------------------------------------
// BIOPStreamEventMessage - decodeEventIds
//----------------------------------------------------------------------------

void ts::BIOPStreamEventMessage::decodeEventIds()
{
    // object_info for StreamEvent:
    //   DSM::Event::Info {
    //     aDescription (info_length + StreamInfo) -- skip
    //     eventIds_count  uint16
    //     eventId[]       uint16[]
    //   }
    // We need to skip the aDescription part first.
    event_ids.clear();
    if (object_info.size() < 4) {
        return;  // Too short for even the info_length field
    }
    const uint32_t info_length = GetUInt32(object_info.data());
    const size_t event_offset = 4 + info_length;
    if (object_info.size() < event_offset + 2) {
        return;
    }
    const uint16_t count = GetUInt16(object_info.data() + event_offset);
    const size_t ids_offset = event_offset + 2;
    for (uint16_t i = 0; i < count && ids_offset + 2 * i + 1 < object_info.size(); ++i) {
        event_ids.push_back(GetUInt16(object_info.data() + ids_offset + 2 * i));
    }
}


//----------------------------------------------------------------------------
// BIOPStreamEventMessage - deserializeBody
//----------------------------------------------------------------------------

bool ts::BIOPStreamEventMessage::deserializeBody(PSIBuffer& buf)
{
    // The Stream part (aDescription + taps)
    if (!BIOPStreamMessage::deserializeBody(buf)) {
        return false;
    }

    // Event names
    const uint8_t names_count = buf.getUInt8();
    for (uint8_t i = 0; i < names_count && !buf.error(); ++i) {
        const uint8_t len = buf.getUInt8();
        const ByteBlock raw = buf.getBytes(len);
        event_names.push_back(UString::FromUTF8(reinterpret_cast<const char*>(raw.data()), raw.size()));
    }

    // Decode event IDs from object_info
    decodeEventIds();

    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPStreamEventMessage - toXMLBody / fromXMLBody
//----------------------------------------------------------------------------

void ts::BIOPStreamEventMessage::toXMLBody(DuckContext& duck, xml::Element* msg_element) const
{
    // Serialize the stream part first
    BIOPStreamMessage::toXMLBody(duck, msg_element);

    // Add event IDs and names
    xml::Element* body = msg_element->findFirstChild(u"stream_body", true);
    if (body != nullptr) {
        for (uint16_t id : event_ids) {
            body->addElement(u"event_id")->setIntAttribute(u"value", id, true);
        }
        for (const auto& name : event_names) {
            body->addElement(u"event_name")->setAttribute(u"value", name);
        }
    }
}

bool ts::BIOPStreamEventMessage::fromXMLBody(DuckContext& duck, const xml::Element* msg_element)
{
    if (!BIOPStreamMessage::fromXMLBody(duck, msg_element)) {
        return false;
    }
    const xml::Element* body = msg_element->findFirstChild(u"stream_body", true);
    if (body == nullptr) {
        return false;
    }
    bool ok = true;
    for (auto& xe : body->children(u"event_id", &ok)) {
        uint16_t id = 0;
        ok = ok && xe.getIntAttribute(id, u"value", true);
        if (ok) {
            event_ids.push_back(id);
        }
    }
    for (auto& xe : body->children(u"event_name", &ok)) {
        UString name;
        ok = ok && xe.getAttribute(name, u"value", true);
        if (ok) {
            event_names.push_back(name);
        }
    }
    return ok;
}
