/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include "GoToMonitorObject.h"

#include <GoTo.h>
#include <GoToStatus.h>
// Include also the .cpp file in order to use the internal types
#include <GoTo.cpp>
#include <GoToStatus.cpp>

#include <yarp/os/LogComponent.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/Things.h>

namespace {
YARP_LOG_COMPONENT(GOTOMONITOR,
                   "scope.carrier.portmonitor.GoTo",
                   yarp::os::Log::minimumPrintLevel(),
                   yarp::os::Log::LogTypeReserved,
                   yarp::os::Log::printCallback(),
                   nullptr)
}

bool GoToMonitorObject::create(const yarp::os::Property& options)
{
    sender = options.find("sender_side").asBool();
    if (!sender) {
        yCError(GOTOMONITOR) << "Attaching on receiver side is not supported yet.";
        return false;
    }

    source = options.find("source").asString();
    destination = options.find("destination").asString();

    if (!port.open/*Fake*/(source + "/monitor")) {
        return false;
    }

    if (!port.addOutput("/monitor")) {
        return false;
    }

    return true;
}

yarp::os::Things& GoToMonitorObject::update(yarp::os::Things& thing)
{
    std::lock_guard<std::mutex> lock(mutex);

    yCTrace(GOTOMONITOR) << "update()";

    yarp::os::Bottle msg;
    msg.addFloat64(yarp::os::SystemClock::nowSystem());
    msg.addString(source);
    msg.addString(destination);
    msg.addString("command");
    //msg.addBool(sender);
    auto& bcmd = msg.addList();


#if 0
    if (!sender) {
        yCTrace(GOTOMONITOR) << "update() -> receiver";
        const auto* cmd = thing.cast_as<yarp::os::Bottle>();
        if (cmd) {
            yCInfo(GOTOMONITOR) << "Received command:" << cmd->toString();
        }
        return thing;
    }
# endif
    yCTrace(GOTOMONITOR) << "update() -> sender";
//     yarp::os::Portable::copyPortable(*(thing.getPortWriter()), data);
//     yCInfo(GOTOMONITOR) << "Sending command:" << data.toString();

    if (const auto* helper = thing.cast_as<GoTo_goTo_helper>()) {
        yCDebug(GOTOMONITOR) << "Sending command 'goTo'" << helper->cmd.destination;
        bcmd.addString("goTo");
        bcmd.addString(helper->cmd.destination);
    } else if (const auto* cmd = thing.cast_as<GoTo_getStatus_helper>()) {
        yCDebug(GOTOMONITOR) << "Sending command 'getStatus'" << helper->cmd.destination;
        bcmd.addString("getStatus");
        bcmd.addString(helper->cmd.destination);
    } else if (const auto* cmd = thing.cast_as<GoTo_halt_helper>()) {
        yCDebug(GOTOMONITOR) << "Sending command 'halt'" << helper->cmd.destination;
        bcmd.addString("halt");
        bcmd.addString(helper->cmd.destination);
    } else if (const auto* cmd = thing.cast_as<GoTo_isAtLocation_helper>()) {
        yCDebug(GOTOMONITOR) << "Sending command 'isAtLocation'" << helper->cmd.destination;
        bcmd.addString("isAtLocation");
        bcmd.addString(helper->cmd.destination);
    } else {
        yCWarning(GOTOMONITOR) << "Sending unknown command";
        bcmd.addString("[unknown]");
    }

    yCDebug(GOTOMONITOR, "Writing: %s", msg.toString().c_str());
    port.write(msg);

    return thing;
}



yarp::os::Things& GoToMonitorObject::updateReply(yarp::os::Things& thing)
{
    std::lock_guard<std::mutex> lock(mutex);

    yCTrace(GOTOMONITOR) << "updateReply()";

    yarp::os::Bottle msg;
    msg.addFloat64(yarp::os::SystemClock::nowSystem());
    msg.addString(source);
    msg.addString(destination);
    msg.addString("reply");
    //msg.addBool(sender);
    auto& breply = msg.addList();

#if 0
    if (!sender) {
        yCTrace(GOTOMONITOR) << "update() -> sender";
        yAssert(false); // It doesn't work on receiver side yet
//         yarp::os::Portable::copyPortable(*(thing.getPortWriter()), data);
//         yCInfo(GOTOMONITOR) << "Sending reply:" << data.toString();
        return thing;
    }
#endif

    yCTrace(GOTOMONITOR) << "update() -> receiver";

    // FIXME GoToStatusVocab::toString should be static.
    if (const auto* helper = thing.cast_as<GoTo_getStatus_helper>()) {
        yCDebug(GOTOMONITOR) << "Received reply to 'goTo'" << helper->cmd.destination;
        breply.addString("goTo");
        breply.addString(helper->cmd.destination);
    } else if (const auto* reply = thing.cast_as<GoTo_getStatus_helper>()) {
        yCDebug(GOTOMONITOR) << "Received reply to 'getStatus'" << helper->cmd.destination << GoToStatusConverter().toString(helper->reply.return_helper);
        breply.addString("getStatus");
        breply.addString(helper->cmd.destination);
        breply.addInt32(static_cast<int32_t>(helper->reply.return_helper));
    } else if (const auto* reply = thing.cast_as<GoTo_halt_helper>()) {
        yCDebug(GOTOMONITOR) << "Received reply to 'halt'" << helper->cmd.destination;
        breply.addString("halt");
        breply.addString(helper->cmd.destination);
    } else if (const auto* reply = thing.cast_as<GoTo_isAtLocation_helper>()) {
        yCDebug(GOTOMONITOR) << "Received reply to 'isAtLocation'" << helper->cmd.destination << helper->reply.return_helper;
        breply.addString("isAtLocation");
        breply.addString(helper->cmd.destination);
        breply.addInt32(static_cast<int32_t>(helper->reply.return_helper));
    } else {
        yCWarning(GOTOMONITOR) << "Received unknown reply";
        breply.addString("[unknown]");
    }

    yCDebug(GOTOMONITOR, "Writing: %s", msg.toString().c_str());
    port.write(msg);

    return thing;
}
