// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Live MIDI Input Handler
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "MidiInput.h"
#include "ofMain.h"

namespace avs {

MidiInput::MidiInput() : midi_in_("ofxAVS MIDI") {
}

MidiInput::~MidiInput() {
    closeDevice();
}

std::vector<std::string> MidiInput::getDeviceList() {
    return midi_in_.getInPortList();
}

bool MidiInput::openDevice(int index) {
    closeDevice();
    if (midi_in_.openPort(index)) {
        addLogEntry("Opened: " + midi_in_.getName());
        return true;
    }
    return false;
}

bool MidiInput::openDevice(const std::string& name) {
    closeDevice();
    if (midi_in_.openPort(name)) {
        addLogEntry("Opened: " + midi_in_.getName());
        return true;
    }
    return false;
}

void MidiInput::closeDevice() {
    if (midi_in_.isOpen()) {
        addLogEntry("Closed: " + midi_in_.getName());
        midi_in_.closePort();
    }
}

bool MidiInput::isOpen() {
    return midi_in_.isOpen();
}

std::string MidiInput::getDeviceName() {
    return midi_in_.getName();
}

void MidiInput::setChannel(int channel) {
    channel_ = std::clamp(channel, 0, 16);
}

void MidiInput::update() {
    // Process all waiting messages
    ofxMidiMessage msg;
    while (midi_in_.getNextMessage(msg)) {
        processMessage(msg);
    }
}

void MidiInput::processMessage(const ofxMidiMessage& msg) {
    // Channel filter (msg.channel is 1-16)
    if (channel_ != CHANNEL_OMNI && msg.channel != channel_) {
        return;
    }

    // Debug logging
    if (debug_enabled_) {
        addLogEntry(formatMessage(msg));
    }

    // Route to EventBus
    Event event;
    event.channel = msg.channel;
    event.timestamp = ofGetElapsedTimef();

    switch (msg.status) {
        case MIDI_NOTE_ON:
            if (msg.velocity > 0) {
                event.type = Event::Type::MIDI_NOTE_ON;
                event.data1 = msg.pitch;
                event.data2 = msg.velocity;
                EventBus::instance().push_event(event);
            } else {
                // Note on with velocity 0 = note off
                event.type = Event::Type::MIDI_NOTE_OFF;
                event.data1 = msg.pitch;
                event.data2 = 0;
                EventBus::instance().push_event(event);
            }
            break;

        case MIDI_NOTE_OFF:
            event.type = Event::Type::MIDI_NOTE_OFF;
            event.data1 = msg.pitch;
            event.data2 = msg.velocity;
            EventBus::instance().push_event(event);
            break;

        case MIDI_CONTROL_CHANGE:
            event.type = Event::Type::MIDI_CC;
            event.data1 = msg.control;
            event.data2 = msg.value;
            EventBus::instance().push_event(event);
            break;

        case MIDI_PITCH_BEND:
            event.type = Event::Type::MIDI_PITCH_BEND;
            event.data1 = msg.value;  // 0-16383
            event.data2 = 0;
            EventBus::instance().push_event(event);
            break;

        default:
            // Ignore other message types (aftertouch, program change, etc.)
            break;
    }
}

void MidiInput::addLogEntry(const std::string& text) {
    std::lock_guard<std::mutex> lock(log_mutex_);

    MidiLogEntry entry;
    entry.timestamp = ofGetElapsedTimef();
    entry.message = text;

    debug_log_.push_back(entry);

    // Trim to max size
    while (debug_log_.size() > MAX_LOG_ENTRIES) {
        debug_log_.pop_front();
    }
}

void MidiInput::clearDebugLog() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    debug_log_.clear();
}

std::string MidiInput::formatMessage(const ofxMidiMessage& msg) const {
    std::stringstream ss;
    ss << "Ch" << msg.channel << " ";

    switch (msg.status) {
        case MIDI_NOTE_ON:
            ss << "NoteOn " << msg.pitch << " vel=" << msg.velocity;
            break;
        case MIDI_NOTE_OFF:
            ss << "NoteOff " << msg.pitch << " vel=" << msg.velocity;
            break;
        case MIDI_CONTROL_CHANGE:
            ss << "CC " << msg.control << " val=" << msg.value;
            break;
        case MIDI_PITCH_BEND:
            ss << "PitchBend " << msg.value;
            break;
        case MIDI_AFTERTOUCH:
            ss << "Aftertouch " << msg.value;
            break;
        case MIDI_POLY_AFTERTOUCH:
            ss << "PolyAT " << msg.pitch << " " << msg.value;
            break;
        case MIDI_PROGRAM_CHANGE:
            ss << "PrgChg " << msg.value;
            break;
        default:
            ss << "Status 0x" << std::hex << (int)msg.status;
            break;
    }

    return ss.str();
}

} // namespace avs
