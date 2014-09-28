#include "commands/commands.h"
#include "config.h"
#include "util/log.h"
#include "config.h"
#include "openxc.pb.h"
#include "pb_decode.h"

#include "commands/passthrough_command.h"
#include "commands/diagnostic_request_command.h"
#include "commands/version_command.h"
#include "commands/device_id_command.h"
#include "commands/can_message_write_command.h"
#include "commands/simple_write_command.h"
#include "commands/af_bypass_command.h"
#include "commands/payload_format_command.h"

using openxc::util::log::debug;
using openxc::config::getConfiguration;
using openxc::payload::PayloadFormat;

static bool handleComplexCommand(openxc_VehicleMessage* message) {
    bool status = true;
    if(message != NULL && message->has_control_command) {
        openxc_ControlCommand* command = &message->control_command;
        switch(command->type) {
        case openxc_ControlCommand_Type_DIAGNOSTIC:
            status = openxc::commands::handleDiagnosticRequestCommand(command);
            break;
        case openxc_ControlCommand_Type_VERSION:
            status = openxc::commands::handleVersionCommand();
            break;
        case openxc_ControlCommand_Type_DEVICE_ID:
            status = openxc::commands::handleDeviceIdCommmand();
            break;
        case openxc_ControlCommand_Type_PASSTHROUGH:
            status = openxc::commands::handlePassthroughModeCommand(command);
            break;
        case openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS:
            status = openxc::commands::handleFilterBypassCommand(command);
            break;
        case openxc_ControlCommand_Type_PAYLOAD_FORMAT:
            status = openxc::commands::handlePayloadFormatCommand(command);
            break;
        default:
            status = false;
            break;
        }
    }
    return status;
}

bool openxc::commands::handleIncomingMessage(uint8_t payload[], size_t length) {
    openxc_VehicleMessage message = {0};
    bool status = true;
    if(length > 0 && openxc::payload::deserialize(payload, length,
                getConfiguration()->payloadFormat, &message)) {
        if(validate(&message)) {
            switch(message.type) {
            case openxc_VehicleMessage_Type_RAW:
                handleRaw(&message);
                break;
            case openxc_VehicleMessage_Type_TRANSLATED:
                handleTranslated(&message);
                break;
            case openxc_VehicleMessage_Type_CONTROL_COMMAND:
                handleComplexCommand(&message);
                break;
            default:
                debug("Incoming message had unrecognized type: %d", message.type);
                status = false;
                break;
            }
        } else {
            debug("Incoming message is complete but invalid");
        }
    } else {
        status = false;
    }
    return status;
}

static bool validateControlCommand(openxc_VehicleMessage* message) {
    bool valid = message->has_type &&
            message->type == openxc_VehicleMessage_Type_CONTROL_COMMAND &&
            message->has_control_command &&
            message->control_command.has_type;
    if(valid) {
        switch(message->control_command.type) {
        case openxc_ControlCommand_Type_DIAGNOSTIC:
            valid = openxc::commands::validateDiagnosticRequest(message);
            break;
        case openxc_ControlCommand_Type_PASSTHROUGH:
            valid = openxc::commands::validatePassthroughRequest(message);
            break;
        case openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS:
            valid = openxc::commands::validateFilterBypassCommand(message);
            break;
        case openxc_ControlCommand_Type_PAYLOAD_FORMAT:
            valid = openxc::commands::validatePayloadFormatCommand(message);
            break;
        case openxc_ControlCommand_Type_VERSION:
        case openxc_ControlCommand_Type_DEVICE_ID:
            valid =  true;
            break;
        default:
            valid = false;
            break;
        }
    }
    return valid;
}

bool openxc::commands::validate(openxc_VehicleMessage* message) {
    bool valid = false;
    if(message != NULL && message->has_type) {
        switch(message->type) {
        case openxc_VehicleMessage_Type_RAW:
            valid = validateRaw(message);
            break;
        case openxc_VehicleMessage_Type_TRANSLATED:
            valid = validateTranslated(message);
            break;
        case openxc_VehicleMessage_Type_CONTROL_COMMAND:
            valid = validateControlCommand(message);
            break;
        default:
            debug("Incoming message had unrecognized type: %d", message->type);
            break;
        }
    }
    return valid;
}
