//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCUserToNetworkMessage.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DSMCC_user_to_network_message"
#define MY_CLASS    ts::DSMCCUserToNetworkMessage
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::MPEG

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::DSMCCUserToNetworkMessage::DSMCCUserToNetworkMessage(uint8_t vers, bool cur, uint16_t tid_ext) :
    AbstractDSMCCTable(MY_TID, MY_XML_NAME, MY_STD, tid_ext, vers, cur),
    modules(this)
{
}

ts::DSMCCUserToNetworkMessage::DSMCCUserToNetworkMessage(DuckContext& duck, const BinaryTable& table) :
    DSMCCUserToNetworkMessage(0, true)
{
    deserialize(duck, table);
}

ts::DSMCCUserToNetworkMessage::Module::Module(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}

void ts::DSMCCUserToNetworkMessage::clearContent()
{
    // DSM-CC Message Header
    _header.clear();

    // DSI
    server_id.clear();

    // DII
    download_id = 0;
    block_size = 0;
    modules.clear();
}

//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

uint16_t ts::DSMCCUserToNetworkMessage::tableIdExtension() const
{
    return uint16_t(_header.download_transaction_id & 0xFFFF);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessage::deserializePayload(PSIBuffer& buf, const Section& section)
{
    AbstractDSMCCTable::deserializePayload(buf, section);

    buf.skipBytes(1);

    const uint8_t adaptation_length = buf.getUInt8();

    buf.skipBytes(2);  // message_length

    // For object carousel it should be 0
    if (adaptation_length > 0) {
        buf.setUserError();
        buf.skipBytes(adaptation_length);
    }

    if (isDSI(_header.message_id)) {
        buf.getBytes(server_id, SERVER_ID_SIZE);

        buf.skipBytes(2);  // compatibility_descriptor_length
        buf.skipBytes(2);  // private_data_length

        uint32_t type_id_length = buf.getUInt32();

        for (size_t i = 0; i < type_id_length; i++) {
            ior.type_id.appendUInt8(buf.getUInt8());
        }

        // CDR alligment rule
        if (type_id_length % 4 != 0) {
            buf.skipBytes(4 - (type_id_length % 4));
        }

        const uint32_t tagged_profiles_count = buf.getUInt32();

        for (size_t i = 0; i < tagged_profiles_count; i++) {
            TaggedProfile tagged_profile;

            tagged_profile.profile_id_tag = buf.getUInt32();

            const uint32_t profile_data_length = buf.getUInt32();

            tagged_profile.profile_data_byte_order = buf.getUInt8();

            if (tagged_profile.profile_id_tag == DSMCC_TAG_BIOP_PROFILE) {  // TAG_BIOP (BIOP Profile Body)
                const uint8_t lite_component_count = buf.getUInt8();

                for (size_t j = 0; j < lite_component_count; j++) {

                    const uint32_t component_id_tag = buf.getUInt32();
                    const uint8_t  component_data_length = buf.getUInt8();

                    switch (component_id_tag) {
                        case DSMCC_TAG_OBJECT_LOCATION: {  // TAG_ObjectLocation

                            LiteComponent biopObjectLocation;

                            biopObjectLocation.component_id_tag = component_id_tag;
                            biopObjectLocation.carousel_id = buf.getUInt32();
                            biopObjectLocation.module_id = buf.getUInt16();
                            biopObjectLocation.version_major = buf.getUInt8();
                            biopObjectLocation.version_minor = buf.getUInt8();
                            const uint8_t object_key_length = buf.getUInt8();

                            for (size_t k = 0; k < object_key_length; k++) {
                                biopObjectLocation.object_key_data.appendUInt8(buf.getUInt8());
                            }

                            tagged_profile.liteComponents.push_back(biopObjectLocation);

                            break;
                        }

                        case DSMCC_TAG_CONN_BINDER: {  // TAG_ConnBinder

                            LiteComponent dsmConnBinder;

                            dsmConnBinder.component_id_tag = component_id_tag;

                            buf.skipBytes(1);  // taps_count

                            dsmConnBinder.tap.id = buf.getUInt16();
                            dsmConnBinder.tap.use = buf.getUInt16();
                            dsmConnBinder.tap.association_tag = buf.getUInt16();

                            buf.skipBytes(1);  // selector_length

                            dsmConnBinder.tap.selector_type = buf.getUInt16();
                            dsmConnBinder.tap.transaction_id = buf.getUInt32();
                            dsmConnBinder.tap.timeout = buf.getUInt32();

                            for (int n = 0; n < component_data_length - 18; n++) {
                                buf.skipBytes(1);  // selector_data
                            }

                            tagged_profile.liteComponents.push_back(dsmConnBinder);

                            break;
                        }

                        default: {
                            LiteComponent unknownComponent;

                            unknownComponent.component_id_tag = component_id_tag;

                            buf.getBytes(unknownComponent.component_data.value(), component_data_length);

                            tagged_profile.liteComponents.push_back(unknownComponent);

                            break;
                        }
                    }
                }
            }
            else if (tagged_profile.profile_id_tag == DSMCC_TAG_LITE_OPTIONS) {  // TAG_LITE_OPTIONS (Lite Options Profile Body)
                buf.getBytes(tagged_profile.profile_data.value(), profile_data_length - 1);
            }
            else {
                buf.getBytes(tagged_profile.profile_data.value(), profile_data_length - 1);
            }

            ior.tagged_profiles.push_back(tagged_profile);
        }

        buf.skipBytes(4);  // download_taps_count + service_context_list_count + user_info_length
    }
    else if (isDII(_header.message_id)) {

        download_id = buf.getUInt32();
        block_size = buf.getUInt16();

        buf.skipBytes(10);  // windowSize + ackPeriod + tCDownloadWindow + tCDownloadScenario
        buf.skipBytes(2);   // compatibility_descriptor_length

        const uint16_t number_of_modules = buf.getUInt16();

        for (size_t i = 0; i < number_of_modules; i++) {
            Module& module(modules.newEntry());

            module.module_id = buf.getUInt16();
            module.module_size = buf.getUInt32();
            module.module_version = buf.getUInt8();

            buf.skipBytes(1);  // module_info_length

            module.module_timeout = buf.getUInt32();
            module.block_timeout = buf.getUInt32();
            module.min_block_time = buf.getUInt32();

            const uint8_t taps_count = buf.getUInt8();

            for (size_t j = 0; j < taps_count; j++) {
                Tap tap;

                tap.id = buf.getUInt16();
                tap.use = buf.getUInt16();
                tap.association_tag = buf.getUInt16();

                buf.skipBytes(1);  // selector_length

                module.taps.push_back(tap);
            }

            uint8_t user_info_length = buf.getUInt8();

            buf.getDescriptorList(module.descs, user_info_length);
        }

        uint16_t private_data_length = buf.getUInt16();

        buf.skipBytes(private_data_length);
    }
    else {

        buf.setUserError();
        buf.skipBytes(buf.remainingReadBytes());
    }
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------
void ts::DSMCCUserToNetworkMessage::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // DSMCC_UNM Table consist only one section so we do not need to worry about overflow
    AbstractDSMCCTable::serializePayload(table, buf);

    buf.putUInt8(0xFF);  // reserved
    buf.putUInt8(0x00);  // adaptation_length

    buf.pushWriteSequenceWithLeadingLength(16);

    if (isDSI(_header.message_id)) {
        buf.putBytes(server_id);
        buf.putUInt16(0x0000);  // compatibility_descriptor_length

        buf.pushWriteSequenceWithLeadingLength(16);  // private_data

        // IOP::IOR
        buf.putUInt32(uint32_t(ior.type_id.size()));
        buf.putBytes(ior.type_id);

        buf.putUInt32(uint32_t(ior.tagged_profiles.size()));

        for (const auto& tagged_profile : ior.tagged_profiles) {
            buf.putUInt32(tagged_profile.profile_id_tag);

            buf.pushWriteSequenceWithLeadingLength(32);  // profile_data

            buf.putUInt8(tagged_profile.profile_data_byte_order);

            if (tagged_profile.profile_id_tag == DSMCC_TAG_BIOP_PROFILE) {  // TAG_BIOP (BIOP Profile Body)

                buf.putUInt8(uint8_t(tagged_profile.liteComponents.size()));

                for (const auto& liteComponent : tagged_profile.liteComponents) {
                    buf.putUInt32(liteComponent.component_id_tag);

                    buf.pushWriteSequenceWithLeadingLength(8);  // component_data

                    switch (liteComponent.component_id_tag) {
                        case DSMCC_TAG_OBJECT_LOCATION: {  // TAG_ObjectLocation
                            buf.putUInt32(liteComponent.carousel_id);
                            buf.putUInt16(liteComponent.module_id);
                            buf.putUInt8(liteComponent.version_major);
                            buf.putUInt8(liteComponent.version_minor);
                            buf.putUInt8(uint8_t(liteComponent.object_key_data.size()));
                            buf.putBytes(liteComponent.object_key_data);
                            break;
                        }

                        case DSMCC_TAG_CONN_BINDER: {  // TAG_ConnBinder
                            buf.putUInt8(0x01);        // taps_count TODO: for now only one tap assumed but this needs to be
                                                       // fixed
                            buf.putUInt16(liteComponent.tap.id);
                            buf.putUInt16(liteComponent.tap.use);
                            buf.putUInt16(liteComponent.tap.association_tag);
                            buf.putUInt8(0x0A);  // selector_length fixed
                            buf.putUInt16(liteComponent.tap.selector_type);
                            buf.putUInt32(liteComponent.tap.transaction_id);
                            buf.putUInt32(liteComponent.tap.timeout);
                            break;
                        }

                        default: {  // UnknownComponent
                            if (liteComponent.component_data.has_value()) {
                                buf.putBytes(liteComponent.component_data.value());
                            }
                        }
                    }

                    buf.popState();  // close component_data
                }
            }
            else if (tagged_profile.profile_id_tag == DSMCC_TAG_LITE_OPTIONS) {  // TAG_LITE_OPTIONS (Lite Options Profile Body)
                if (tagged_profile.profile_data.has_value()) {
                    buf.putBytes(tagged_profile.profile_data.value());
                }
            }
            else {
                if (tagged_profile.profile_data.has_value()) {
                    buf.putBytes(tagged_profile.profile_data.value());
                }
            }

            buf.popState();  // close profile_data
        }

        buf.putUInt8(0x00);     // download_taps_count
        buf.putUInt8(0x00);     // service_context_list_count
        buf.putUInt16(0x0000);  // user_info_length

        buf.popState();  // close private_data
    }
    else if (isDII(_header.message_id)) {

        buf.putUInt32(download_id);
        buf.putUInt16(block_size);

        // ETSI TR 101 202 V1.2.1 5.7.5.1
        // Not used and set to zero
        buf.putUInt8(0x00);         // windowSize
        buf.putUInt8(0x00);         // ackPeriod
        buf.putUInt32(0x00000000);  // tCDownloadWindow
        buf.putUInt32(0x00000000);  // tCDownloadScenario
        buf.putUInt16(0x0000);      // compatibility_descriptor_length

        buf.putUInt16(uint16_t(modules.size()));

        for (const auto& module : modules) {

            buf.putUInt16(module.second.module_id);
            buf.putUInt32(module.second.module_size);
            buf.putUInt8(module.second.module_version);

            buf.pushWriteSequenceWithLeadingLength(8);  // module_info_length

            // BIOP::ModuleInfo

            buf.putUInt32(module.second.module_timeout);
            buf.putUInt32(module.second.block_timeout);
            buf.putUInt32(module.second.min_block_time);

            buf.putUInt8(uint8_t(module.second.taps.size()));  // taps_count

            for (const auto& tap : module.second.taps) {
                buf.putUInt16(tap.id);
                buf.putUInt16(tap.use);
                buf.putUInt16(tap.association_tag);
                buf.putUInt8(0x00);  // selector_length
            }

            buf.pushWriteSequenceWithLeadingLength(8);  // user_info_length

            // Normaly I would use putDescriptorListWithLength
            // but UserInfoLength is only 1 byte instead of 2 asssumed
            // by above method
            buf.putDescriptorList(module.second.descs);

            buf.popState();  // close user_info_length

            buf.popState();  // close module_info_length
        }

        buf.putUInt16(0x0000);  // private_data_length
    }
    buf.popState();  // close message_length
}

void ts::DSMCCUserToNetworkMessage::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    uint8_t  adaptation_length = 0;
    uint16_t message_id = 0;

    if (buf.canReadBytes(DSMCC_MESSAGE_HEADER_SIZE)) {
        const uint8_t protocol_discriminator = buf.getUInt8();
        const uint8_t dsmcc_type = buf.getUInt8();
        message_id = buf.getUInt16();
        const uint32_t transaction_id = buf.getUInt32();

        buf.skipBytes(1);  // reserved

        adaptation_length = buf.getUInt8();

        buf.skipBytes(2);  // message_length

        // For object carousel it should be 0
        if (adaptation_length > 0) {
            buf.skipBytes(adaptation_length);
        }

        disp << margin << UString::Format(u"Protocol discriminator: %n", protocol_discriminator) << std::endl;
        disp << margin << "Dsmcc type: " << DataName(MY_XML_NAME, u"dsmcc_type", dsmcc_type, NamesFlags::HEX_VALUE_NAME) << std::endl;
        if (dsmcc_type == 0x03) {
            disp << margin << "Message id: " << DataName(MY_XML_NAME, u"message_id", message_id, NamesFlags::HEX_VALUE_NAME) << std::endl;
        }
        else {
            disp << margin << UString::Format(u"Message id: %n", message_id) << std::endl;
        }
        disp << margin << UString::Format(u"Transaction id: %n", transaction_id) << std::endl;
    }

    if (isDSI(message_id)) {  // DSI
        disp.displayPrivateData(u"Server id", buf, SERVER_ID_SIZE, margin);

        buf.skipBytes(2);  // compatibility_descriptor_length shall be 0x0000
        buf.skipBytes(2);  // private_data_length

        uint32_t  type_id_length = buf.getUInt32();
        ByteBlock type_id {};

        for (size_t i = 0; i < type_id_length; i++) {
            type_id.appendUInt8(buf.getUInt8());
        }

        disp.displayVector(u"Type id: ", type_id, margin);

        uint32_t tagged_profiles_count = buf.getUInt32();

        for (size_t i = 0; i < tagged_profiles_count; i++) {

            uint32_t profile_id_tag = buf.getUInt32();
            uint32_t profile_data_length = buf.getUInt32();
            uint8_t  profile_data_byte_order = buf.getUInt8();

            disp << margin << "ProfileId Tag: " << DataName(MY_XML_NAME, u"tag", profile_id_tag, NamesFlags::HEX_VALUE_NAME) << std::endl;
            disp << margin << UString::Format(u"Profile Data Byte Order: %n", profile_data_byte_order) << std::endl;

            if (profile_id_tag == DSMCC_TAG_BIOP_PROFILE) {  // TAG_BIOP (BIOP Profile Body)
                uint8_t lite_component_count = buf.getUInt8();
                disp << margin << UString::Format(u"Lite Component Count: %n", lite_component_count) << std::endl;

                for (size_t j = 0; j < lite_component_count; j++) {

                    uint32_t componentid_tag = buf.getUInt32();
                    uint8_t  component_data_length = buf.getUInt8();

                    disp << margin << "ComponentId Tag: " << DataName(MY_XML_NAME, u"tag", componentid_tag, NamesFlags::HEX_VALUE_NAME) << std::endl;

                    switch (componentid_tag) {
                        case DSMCC_TAG_OBJECT_LOCATION: {  // TAG_ObjectLocation

                            uint32_t  carousel_id = buf.getUInt32();
                            uint16_t  module_id = buf.getUInt16();
                            uint8_t   version_major = buf.getUInt8();
                            uint8_t   version_minor = buf.getUInt8();
                            uint8_t   object_key_length = buf.getUInt8();
                            ByteBlock object_key_data {};

                            for (size_t k = 0; k < object_key_length; k++) {
                                object_key_data.appendUInt8(buf.getUInt8());
                            }

                            disp << margin << UString::Format(u"Carousel Id: %n", carousel_id) << std::endl;
                            disp << margin << UString::Format(u"Module Id: %n", module_id) << std::endl;
                            disp << margin << UString::Format(u"Version Major: %n", version_major) << std::endl;
                            disp << margin << UString::Format(u"Version Minor: %n", version_minor) << std::endl;

                            disp.displayVector(u"Object Key Data: ", object_key_data, margin);

                            break;
                        }

                        case DSMCC_TAG_CONN_BINDER: {  // TAG_ConnBinder

                            uint8_t taps_count = buf.getUInt8();

                            for (size_t k = 0; k < taps_count; k++) {

                                uint16_t id = buf.getUInt16();
                                uint16_t use = buf.getUInt16();
                                uint16_t association_tag = buf.getUInt16();

                                buf.skipBytes(1);  // selector_length

                                uint16_t selector_type = buf.getUInt16();
                                uint32_t transaction_id = buf.getUInt32();
                                uint32_t timeout = buf.getUInt32();

                                disp << margin << UString::Format(u"Tap id: %n", id) << std::endl;
                                disp << margin << "Tap use: " << DataName(MY_XML_NAME, u"tap_use", use, NamesFlags::HEX_VALUE_NAME) << std::endl;
                                disp << margin << UString::Format(u"Tap association tag: %n", association_tag) << std::endl;
                                disp << margin << UString::Format(u"Tap selector type: %n", selector_type) << std::endl;
                                disp << margin << UString::Format(u"Tap transaction id: %n", transaction_id) << std::endl;
                                disp << margin << UString::Format(u"Tap timeout: %n", timeout) << std::endl;
                            }
                            break;
                        }

                        default: {
                            disp.displayPrivateData(u"Lite Component Data", buf, component_data_length, margin);
                            break;
                        }
                    }
                }
            }
            else if (profile_id_tag == DSMCC_TAG_LITE_OPTIONS) {  // TAG_LITE_OPTIONS (Lite Options Profile Body)
                disp.displayPrivateData(u"Lite Options Profile Body Data", buf, profile_data_length - 1, margin);
            }
            else {
                disp.displayPrivateData(u"Unknown Profile Data", buf, profile_data_length - 1, margin);
            }
        }

        uint8_t  download_taps_count = buf.getUInt8();
        uint8_t  service_context_list_count = buf.getUInt8();
        uint16_t user_info_length = buf.getUInt16();

        disp << margin << UString::Format(u"Download taps count: %n", download_taps_count) << std::endl;
        disp << margin << UString::Format(u"Service context list count: %n", service_context_list_count) << std::endl;
        disp << margin << UString::Format(u"User info length: %n", user_info_length) << std::endl;
    }
    else if (isDII(message_id)) {  //DII
        disp << margin << UString::Format(u"Download id: %n", buf.getUInt32()) << std::endl;
        disp << margin << UString::Format(u"Block size: %n", buf.getUInt16()) << std::endl;

        buf.skipBytes(10);  // windowSize + ackPeriod + tCDownloadWindow + tCDownloadScenario
        buf.skipBytes(2);   // compatibility_descriptor_length

        uint16_t number_of_modules = buf.getUInt16();

        for (size_t i = 0; i < number_of_modules; i++) {

            uint16_t module_id = buf.getUInt16();
            uint32_t module_size = buf.getUInt32();
            uint8_t  module_version = buf.getUInt8();

            disp << margin << UString::Format(u"Module id: %n", module_id) << std::endl;
            disp << margin << UString::Format(u"Module size: %n", module_size) << std::endl;
            disp << margin << UString::Format(u"Module version: %n", module_version) << std::endl;

            buf.skipBytes(1);  // module_info_length

            uint32_t module_timeout = buf.getUInt32();
            uint32_t block_timeout = buf.getUInt32();
            uint32_t min_block_time = buf.getUInt32();
            uint8_t  taps_count = buf.getUInt8();

            disp << margin << UString::Format(u"Module timeout: %n", module_timeout) << std::endl;
            disp << margin << UString::Format(u"Block timeout: %n", block_timeout) << std::endl;
            disp << margin << UString::Format(u"Min block time: %n", min_block_time) << std::endl;
            disp << margin << UString::Format(u"Taps count: %n", taps_count) << std::endl;

            for (size_t k = 0; k < taps_count; k++) {

                uint16_t  id = buf.getUInt16();
                uint16_t  use = buf.getUInt16();
                uint16_t  association_tag = buf.getUInt16();
                uint8_t   selector_length = buf.getUInt8();
                ByteBlock selector_type {};

                for (size_t l = 0; l < selector_length; l++) {
                    selector_type.appendUInt8(buf.getUInt8());
                }

                disp << margin << UString::Format(u"Tap id: %n", id) << std::endl;
                disp << margin << "Tap use: " << DataName(MY_XML_NAME, u"tap_use", use, NamesFlags::HEX_VALUE_NAME) << std::endl;
                disp << margin << UString::Format(u"Tap association tag: %n", association_tag) << std::endl;
                if (selector_length > 0) {
                    disp.displayVector(u"Selector type: ", selector_type, margin);
                }
            }

            uint8_t user_info_length = buf.getUInt8();

            DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards());
            disp.displayDescriptorList(section, context, false, buf, margin, u"Descriptor List:", u"None", user_info_length);
        }

        uint16_t private_data_length = buf.getUInt16();
        disp.displayPrivateData(u"Private data", buf, private_data_length, margin);
    }
    else {
        buf.setUserError();
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}

//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCUserToNetworkMessage::buildXML(DuckContext& duck, xml::Element* root) const
{
    AbstractDSMCCTable::buildXML(duck, root);

    if (isDSI(_header.message_id)) {
        xml::Element* dsi = root->addElement(u"DSI");
        dsi->addHexaTextChild(u"server_id", server_id, true);

        xml::Element* ior_entry = dsi->addElement(u"IOR");
        ior_entry->addHexaTextChild(u"type_id", ior.type_id, true);

        for (const auto& profile : ior.tagged_profiles) {

            xml::Element* tagged_profile_entry = ior_entry->addElement(u"tagged_profile");

            tagged_profile_entry->setIntAttribute(u"profile_id_tag", profile.profile_id_tag, true);
            tagged_profile_entry->setIntAttribute(u"profile_data_byte_order", profile.profile_data_byte_order, true);

            if (profile.profile_id_tag == DSMCC_TAG_BIOP_PROFILE) {

                xml::Element* biop_profile_body_entry = tagged_profile_entry->addElement(u"BIOP_profile_body");

                for (const auto& liteComponent : profile.liteComponents) {

                    xml::Element* lite_component_entry = biop_profile_body_entry->addElement(u"lite_component");
                    lite_component_entry->setIntAttribute(u"component_id_tag", liteComponent.component_id_tag, true);

                    switch (liteComponent.component_id_tag) {
                        case DSMCC_TAG_OBJECT_LOCATION: {  // TAG_ObjectLocation
                            xml::Element* biop_object_location_entry = lite_component_entry->addElement(u"BIOP_object_location");
                            biop_object_location_entry->setIntAttribute(u"carousel_id", liteComponent.carousel_id, true);
                            biop_object_location_entry->setIntAttribute(u"module_id", liteComponent.module_id, true);
                            biop_object_location_entry->setIntAttribute(u"version_major", liteComponent.version_major, true);
                            biop_object_location_entry->setIntAttribute(u"version_minor", liteComponent.version_minor, true);
                            biop_object_location_entry->addHexaTextChild(u"object_key_data", liteComponent.object_key_data, true);
                            break;
                        }

                        case DSMCC_TAG_CONN_BINDER: {  // TAG_ConnBinder
                            xml::Element* dsm_conn_binder_entry = lite_component_entry->addElement(u"DSM_conn_binder");

                            xml::Element* tap_entry = dsm_conn_binder_entry->addElement(u"BIOP_tap");
                            tap_entry->setIntAttribute(u"id", liteComponent.tap.id, true);
                            tap_entry->setIntAttribute(u"use", liteComponent.tap.use, true);
                            tap_entry->setIntAttribute(u"association_tag", liteComponent.tap.association_tag, true);
                            tap_entry->setIntAttribute(u"selector_type", liteComponent.tap.selector_type, true);
                            tap_entry->setIntAttribute(u"transaction_id", liteComponent.tap.transaction_id, true);
                            tap_entry->setIntAttribute(u"timeout", liteComponent.tap.timeout, true);
                            break;
                        }

                        default: {
                            xml::Element* unknown_component_entry = lite_component_entry->addElement(u"Unknown_component");
                            if (liteComponent.component_data.has_value()) {
                                unknown_component_entry->addHexaTextChild(u"component_data", liteComponent.component_data.value(), true);
                            }
                        }
                    }
                }
            }
            else if (profile.profile_id_tag == DSMCC_TAG_LITE_OPTIONS) {  // TAG_LITE_OPTIONS (Lite Options Profile Body)
                xml::Element* lite_options_profile_entry = tagged_profile_entry->addElement(u"Lite_options_profile_body");

                if (profile.profile_data.has_value()) {
                    lite_options_profile_entry->addHexaTextChild(u"profile_data", profile.profile_data.value(), true);
                }
            }
            else {
                xml::Element* unknown_profile_entry = tagged_profile_entry->addElement(u"Unknown_profile");

                if (profile.profile_data.has_value()) {
                    unknown_profile_entry->addHexaTextChild(u"profile_data", profile.profile_data.value(), true);
                }
            }
        }
    }
    else if (isDII(_header.message_id)) {

        xml::Element* dii = root->addElement(u"DII");
        dii->setIntAttribute(u"download_id", download_id, true);
        dii->setIntAttribute(u"block_size", block_size, true);

        for (const auto& it : modules) {
            xml::Element* mod = dii->addElement(u"module");
            mod->setIntAttribute(u"module_id", it.second.module_id, true);
            mod->setIntAttribute(u"module_size", it.second.module_size, true);
            mod->setIntAttribute(u"module_version", it.second.module_version, true);
            mod->setIntAttribute(u"module_timeout", it.second.module_timeout, true);
            mod->setIntAttribute(u"block_timeout", it.second.block_timeout, true);
            mod->setIntAttribute(u"min_block_time", it.second.min_block_time, true);

            for (const auto& tap : it.second.taps) {
                xml::Element* t = mod->addElement(u"tap");
                t->setIntAttribute(u"id", tap.id, true);
                t->setIntAttribute(u"use", tap.use, true);
                t->setIntAttribute(u"association_tag", tap.association_tag, true);
            }
            it.second.descs.toXML(duck, mod);
        }
    }
}

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCUserToNetworkMessage::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        AbstractDSMCCTable::analyzeXML(duck, element);

    if (ok && isDSI(_header.message_id)) {

        const xml::Element* dsi_element = element->findFirstChild(u"DSI", true);

        ok = dsi_element->getHexaTextChild(server_id, u"server_id");

        const xml::Element* ior_element = dsi_element->findFirstChild(u"IOR", true);
        xml::ElementVector  profile_elements;

        ok = ior_element->getHexaTextChild(ior.type_id, u"type_id") &&
             ior_element->getChildren(profile_elements, u"tagged_profile");

        for (size_t it = 0; ok && it < profile_elements.size(); ++it) {
            TaggedProfile tagged_profile;

            ok = profile_elements[it]->getIntAttribute(tagged_profile.profile_id_tag, u"profile_id_tag", true) &&
                 profile_elements[it]->getIntAttribute(tagged_profile.profile_data_byte_order, u"profile_data_byte_order", true);

            if (tagged_profile.profile_id_tag == DSMCC_TAG_BIOP_PROFILE) {  // TAG_BIOP (BIOP Profile Body)
                const xml::Element* biop_profile_body_element = profile_elements[it]->findFirstChild(u"BIOP_profile_body", true);
                xml::ElementVector  lite_component_elements;

                ok = biop_profile_body_element->getChildren(lite_component_elements, u"lite_component");

                for (size_t it2 = 0; ok && it2 < lite_component_elements.size(); ++it2) {
                    LiteComponent liteComponent;

                    ok = lite_component_elements[it2]->getIntAttribute(liteComponent.component_id_tag, u"component_id_tag", true);

                    switch (liteComponent.component_id_tag) {
                        case DSMCC_TAG_OBJECT_LOCATION: {  // TAG_ObjectLocation
                            const xml::Element* biop_object_location_element = lite_component_elements[it2]->findFirstChild(u"BIOP_object_location", true);

                            ok = biop_object_location_element->getIntAttribute(liteComponent.carousel_id, u"carousel_id", true) &&
                                 biop_object_location_element->getIntAttribute(liteComponent.module_id, u"module_id", true) &&
                                 biop_object_location_element->getIntAttribute(liteComponent.version_major, u"version_major", true) &&
                                 biop_object_location_element->getIntAttribute(liteComponent.version_minor, u"version_minor", true);

                            ByteBlock bb;
                            if (biop_object_location_element->getHexaTextChild(bb, u"object_key_data")) {
                                liteComponent.object_key_data = bb;
                            }

                            break;
                        }

                        case DSMCC_TAG_CONN_BINDER: {  // TAG_ConnBinder
                            const xml::Element* dsm_conn_binder_element = lite_component_elements[it2]->findFirstChild(u"DSM_conn_binder", true);
                            const xml::Element* biop_tap_element = dsm_conn_binder_element->findFirstChild(u"BIOP_tap", true);

                            ok = biop_tap_element->getIntAttribute(liteComponent.tap.id, u"id", true) &&
                                 biop_tap_element->getIntAttribute(liteComponent.tap.use, u"use", true) &&
                                 biop_tap_element->getIntAttribute(liteComponent.tap.association_tag, u"association_tag", true) &&
                                 biop_tap_element->getIntAttribute(liteComponent.tap.selector_type, u"selector_type", true) &&
                                 biop_tap_element->getIntAttribute(liteComponent.tap.transaction_id, u"transaction_id", true) &&
                                 biop_tap_element->getIntAttribute(liteComponent.tap.timeout, u"timeout", true);

                            break;
                        }

                        default: {  //UnknownComponent
                            ByteBlock bb;
                            if (lite_component_elements[it2]->getHexaTextChild(bb, u"component_data")) {
                                liteComponent.component_data = bb;
                            }
                            break;
                        }
                    }

                    if (ok) {
                        tagged_profile.liteComponents.push_back(liteComponent);
                    }
                }
            }
            else if (tagged_profile.profile_id_tag == DSMCC_TAG_LITE_OPTIONS) {  // TODO: TAG_LITE_OPTIONS (Lite Options Profile Body)
                const xml::Element* lite_options_profile_body_element = profile_elements[it]->findFirstChild(u"Lite_options_profile_body", true);

                ByteBlock bb;
                if (lite_options_profile_body_element->getHexaTextChild(bb, u"profile_data")) {
                    tagged_profile.profile_data = bb;
                }
            }
            else {  // Any other Profile Type
                const xml::Element* unknown_profile_body_element = profile_elements[it]->findFirstChild(u"Unknown_profile_body", true);

                ByteBlock bb;
                if (unknown_profile_body_element->getHexaTextChild(bb, u"profile_data")) {
                    tagged_profile.profile_data = bb;
                }
            }

            if (ok) {
                ior.tagged_profiles.push_back(tagged_profile);
            }
        }
    }
    else if (ok && isDII(_header.message_id)) {
        const xml::Element* dii_element = element->findFirstChild(u"DII", true);
        xml::ElementVector  module_elements;

        ok = dii_element->getIntAttribute(download_id, u"download_id", true) &&
             dii_element->getIntAttribute(block_size, u"block_size", true) &&
             dii_element->getChildren(module_elements, u"module");

        for (size_t it = 0; ok && it < module_elements.size(); ++it) {
            Module&            module(modules.newEntry());
            xml::ElementVector unused;
            xml::ElementVector tap_elements;

            ok = module_elements[it]->getIntAttribute(module.module_id, u"module_id", true) &&
                 module_elements[it]->getIntAttribute(module.module_size, u"module_size", true) &&
                 module_elements[it]->getIntAttribute(module.module_version, u"module_version", true) &&
                 module_elements[it]->getIntAttribute(module.module_timeout, u"module_timeout", true) &&
                 module_elements[it]->getIntAttribute(module.block_timeout, u"block_timeout", true) &&
                 module_elements[it]->getIntAttribute(module.min_block_time, u"min_block_time", true) &&
                 module_elements[it]->getChildren(tap_elements, u"tap");

            for (size_t itt = 0; ok && itt < tap_elements.size(); ++itt) {
                Tap tap;

                ok = tap_elements[itt]->getIntAttribute(tap.id, u"id", true) &&
                     tap_elements[itt]->getIntAttribute(tap.use, u"use", true) &&
                     tap_elements[itt]->getIntAttribute(tap.association_tag, u"association_tag", true);

                module.taps.push_back(tap);
            }
            ok = module.descs.fromXML(duck, unused, module_elements[it], u"tap");
        }
    }

    return ok;
}
