//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// Test utility for networking functions.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsArgs.h"
#include "tsCerrReport.h"
#include "tsIPUtils.h"
#include "tsIPAddress.h"
#include "tsUDPSocket.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace ts {
    class NetOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(NetOptions);
    public:
        NetOptions(int argc, char *argv[]);
        virtual ~NetOptions() override;

        bool            local = false;
        bool            no_loopback = false;
        IP              gen = IP::Any;
        UString         send_message {};
        UStringVector   resolve_one {};
        UStringVector   resolve_all {};
        IPSocketAddress udp_send {};
        IPSocketAddress udp_receive {};
    };
}

ts::NetOptions::~NetOptions()
{
}

ts::NetOptions::NetOptions(int argc, char *argv[]) :
    Args(u"Test utility for networking functions", u"[options] ['message-string']")
{
    option(u"", 0, STRING);
    help(u"", u"Message to send with --udp-send and --tcp-send.");

    option(u"udp-receive", 'u', IPSOCKADDR_OA);
    help(u"udp-receive", u"Wait for a message on the specified UDP socket and send a response.");

    option(u"udp-send", 's', IPSOCKADDR);
    help(u"udp-send", u"Send the 'message-string' to the specified socket and wait for a response.");

    option(u"ipv4", '4');
    help(u"ipv4", u"Use only IPv4 addresses.");

    option(u"ipv6", '6');
    help(u"ipv6", u"Use only IPv6 addresses.");

    option(u"no-loopback", 'n');
    help(u"no-loopback", u"With --local, exclude loopback interfaces.");

    option(u"local", 'l');
    help(u"local", u"List local interfaces.");

    option(u"resolve", 'r', STRING, 0, UNLIMITED_COUNT);
    help(u"resolve", u"name", u"Resolve that name once, as in applications.");

    option(u"all-addresses", 'a', STRING, 0, UNLIMITED_COUNT);
    help(u"all-addresses", u"name", u"Get all addresses for that name, as in nslookup.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    if (present(u"ipv4")) {
        gen = IP::v4;
    }
    else if (present(u"ipv6")) {
        gen = IP::v6;
    }
    else {
        gen = IP::Any;
    }
    local = present(u"local");
    no_loopback = present(u"no-loopback");
    getValue(send_message, u"");
    getSocketValue(udp_send, u"udp-send");
    getSocketValue(udp_receive, u"udp-receive");
    getValues(resolve_one, u"resolve");
    getValues(resolve_all, u"all-addresses");

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
// Full image of an IP address.
//----------------------------------------------------------------------------

namespace {
    ts::UString Format(const ts::IPAddress& addr)
    {
        return ts::UString::Format(u"%s: %s (full: \"%s\")", addr.familyName(), addr, addr.toFullString());
    }
}


//----------------------------------------------------------------------------
//  Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    ts::NetOptions opt(argc, argv);
    CERR.setMaxSeverity(opt.maxSeverity());

    // Resolve one host name.
    for (const auto& name : opt.resolve_one) {
        ts::IPAddress addr(opt.gen);
        if (addr.resolve(name, opt)) {
            std::cout << "Resolve \"" << name << "\":" << std::endl;
            std::cout << "  " << Format(addr) << std::endl;
        }
    }

    // Resolve all addresses for one host name.
    for (const auto& name : opt.resolve_all) {
        ts::IPAddressVector addr;
        if (ts::IPAddress::ResolveAllAddresses(addr, name, opt, opt.gen)) {
            std::cout << "Resolve \"" << name << "\":" << std::endl;
            for (const auto& a : addr) {
                std::cout << "  " << Format(a) << std::endl;
            }
        }
    }

    // Get local interfaces.
    if (opt.local) {
        ts::IPAddressMaskVector addr;
        if (ts::GetLocalIPAddresses(addr, !opt.no_loopback, opt.gen, opt)) {
            std::cout << "Local interfaces: " << addr.size() << std::endl;
            for (const auto& a : addr) {
                std::cout << "  " << Format(a) << std::endl;
            }
        }
    }

    // Receive a UDP message, send a response.
    if (opt.udp_receive.hasPort()) {
        ts::UDPSocket sock;
        if (sock.open(opt.gen, opt)) {
            opt.info(u"Waiting on UDP socket %s ...", opt.udp_receive);
            std::string msg(8192, '\0');
            size_t ret_size = 0;
            ts::IPSocketAddress source, destination;
            bool ok =
                sock.reusePort(true, opt) &&
                sock.bind(opt.udp_receive, opt) &&
                sock.receive(msg.data(), msg.size(), ret_size, source, destination, nullptr, opt);
            if (ok) {
                msg.resize(ret_size);
                opt.info(u"Received %d bytes: \"%s\"", ret_size, msg);
                opt.info(u"Source: %s, destination: %s", source, destination);
                msg.insert(0, "-> \"");
                msg.append("\"");
                sock.send(msg.data(), msg.size(), source, opt);
            }
            sock.close(opt);
        }
    }

    // Send a UDP message, wait for the response.
    if (opt.udp_send.hasAddress()) {
        ts::UDPSocket sock;
        if (sock.open(opt.gen, opt)) {
            opt.info(u"Sending to UDP socket %s ...", opt.udp_send);
            std::string msg(opt.send_message.toUTF8());
            bool ok =
                sock.bind(ts::IPSocketAddress::AnySocketAddress(opt.gen), opt) &&
                sock.send(msg.data(), msg.size(), opt.udp_send, opt);
            if (ok) {
                size_t ret_size = 0;
                ts::IPSocketAddress source, destination;
                msg.resize(8192);
                if (sock.receive(msg.data(), msg.size(), ret_size, source, destination, nullptr, opt)) {
                    msg.resize(ret_size);
                    opt.info(u"Received %d bytes: \"%s\"", ret_size, msg);
                    opt.info(u"Source: %s, destination: %s", source, destination);
                }
            }
            sock.close(opt);
        }
    }

    return EXIT_SUCCESS;
}
