//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of a BIOP Control Messages
//!  @see ETSI TR 101 202 V1.2.1 (2003-01), 4.7.3
//!
//----------------------------------------------------------------------------

#pragma once

namespace ts {
    //!
    //! Representation of Tap structure
    //! @see ETSI TR 101 202 V1.2.1 (2003-01), 4.7.2.5
    //!
    class TSDUCKDLL Tap {
    public:
        Tap() = default;                    //!< Default constructor.
        uint16_t id = 0x0000;               //!< This field is for private use (shall be set to zero if not used).
        uint16_t use = 0x0016;              //!< Field indicating the usage of the Tap.
        uint16_t association_tag = 0x0000;  //!< Field to associate the Tap with a particular (Elementary) Stream.

        //! Following fields are only used for the first Tap in DSMConnBinder.
        //! @see ETSI TR 101 202 V1.2.1 (2003-01), 4.7.3.2
        uint16_t selector_type = 0x0001;  //!< Optional selector, to select the associated data from the associated (Elementary) Stream.
        uint32_t transaction_id = 0;      //!< Used for session integrity and error processing.
        uint32_t timeout = 0;             //!< Defined in units of µs, specific to the construction of a particular carousel.
    };

    //!
    //! Representation of LiteComponent structure (BIOP::Object Location, DSM::ConnBinder)
    //! @see ETSI TR 101 202 V1.2.1 (2003-01), Table 4.5
    //!
    class TSDUCKDLL LiteComponent {
    public:
        LiteComponent() = default;      //!< Default constructor.
        uint32_t component_id_tag = 0;  //!< Component idenfitier tag (eg. TAG_ObjectLocation, TAG_ConnBinder).

        // BIOPObjectLocation context
        uint32_t  carousel_id = 0;       //!< The carouselId field provides a context for the moduleId field.
        uint16_t  module_id = 0;         //!< Identifies the module in which the object is conveyed within the carousel.
        uint8_t   version_major = 0x01;  //!< Fixed, BIOP protocol major version 1.
        uint8_t   version_minor = 0x00;  //!< Fixed, BIOP protocol minor version 0.
        ByteBlock object_key_data {};    //!< Identifies the object within the module in which it is broadcast.

        // DSMConnBinder context
        Tap tap {};  //!< Tap structure

        // UnknownComponent context
        std::optional<ByteBlock> component_data {};  //!< Optional component data, for UnknownComponent.
    };

    //!
    //! Representation of TaggedProfile structure (BIOP Profile Body, Lite Options Profile Body)
    //! @see ETSI TR 101 202 V1.2.1 (2003-01), 4.7.3.2, 4.7.3.3
    //!
    class TSDUCKDLL TaggedProfile {
    public:
        TaggedProfile() = default;             //!< Default constructor.
        uint32_t profile_id_tag = 0;           //!< Profile identifier tag (eg. TAG_BIOP, TAG_LITE_OPTIONS).
        uint8_t  profile_data_byte_order = 0;  //!< Fixed 0x00, big endian byte order.

        // BIOP Profile Body context
        std::list<LiteComponent> liteComponents {};  //!< List of LiteComponent.

        // Any other profile context for now
        std::optional<ByteBlock> profile_data {};  //!< Optional profile data, for UnknownProfile.
    };

    //!
    //! Representation of Interoperable Object Reference (IOR) structure
    //! @see ETSI TR 101 202 V1.2.1 (2003-01), 4.7.3.1
    //!
    class TSDUCKDLL IOR {
    public:
        IOR() = default;                              //!< Default constructor.
        ByteBlock                type_id {};          //!< U-U Objects type_id.
        std::list<TaggedProfile> tagged_profiles {};  //!< List of tagged profiles.
    };

    static constexpr size_t SERVER_ID_SIZE = 20;  //!< Fixed size in bytes of server_id.

    static constexpr uint32_t DSMCC_TAG_LITE_OPTIONS = 0x49534F05;     //!< TAG_LITE_OPTIONS (Lite Options Profile Body).
    static constexpr uint32_t DSMCC_TAG_BIOP_PROFILE = 0x49534F06;     //!< TAG_BIOP (BIOP Profile Body).
    static constexpr uint32_t DSMCC_TAG_CONN_BINDER = 0x49534F40;      //!< TAG_ConnBinder (DSM::ConnBinder).
    static constexpr uint32_t DSMCC_TAG_OBJECT_LOCATION = 0x49534F50;  //!< TAG_ObjectLocation (BIOP::ObjectLocation).
}  // namespace ts
