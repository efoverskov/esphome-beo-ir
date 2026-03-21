#pragma once

#include <cstdint>

// B&O address codes
enum BeoAddress : uint8_t {
  BEO_ADDR_VIDEO     = 0x00,
  BEO_ADDR_AUDIO     = 0x01,
  BEO_ADDR_VIDEOTAPE = 0x05,
  BEO_ADDR_ALL       = 0x0F,
  BEO_ADDR_LIGHT     = 0x1B,
  BEO_ADDR_SPDEMO    = 0x1D,
  BEO_ADDR_NAV       = 0xA1,
};

// B&O command codes (from Beomote Commands.h)
enum BeoCommand : uint8_t {
  BEO_CMD_NUMBER_0      = 0x00,
  BEO_CMD_NUMBER_1      = 0x01,
  BEO_CMD_NUMBER_2      = 0x02,
  BEO_CMD_NUMBER_3      = 0x03,
  BEO_CMD_NUMBER_4      = 0x04,
  BEO_CMD_NUMBER_5      = 0x05,
  BEO_CMD_NUMBER_6      = 0x06,
  BEO_CMD_NUMBER_7      = 0x07,
  BEO_CMD_NUMBER_8      = 0x08,
  BEO_CMD_NUMBER_9      = 0x09,
  BEO_CMD_CLEAR         = 0x0A,
  BEO_CMD_STORE         = 0x0B,
  BEO_CMD_STANDBY       = 0x0C,
  BEO_CMD_MUTE          = 0x0D,
  BEO_CMD_INDEX         = 0x0E,
  BEO_CMD_UP            = 0x1E,
  BEO_CMD_DOWN          = 0x1F,
  BEO_CMD_TUNE          = 0x20,
  BEO_CMD_CLOCK         = 0x28,
  BEO_CMD_FORMAT        = 0x2A,
  BEO_CMD_LEFT          = 0x32,
  BEO_CMD_RETURN        = 0x33,
  BEO_CMD_RIGHT         = 0x34,
  BEO_CMD_GO            = 0x35,
  BEO_CMD_STOP          = 0x36,
  BEO_CMD_RECORD        = 0x37,
  BEO_CMD_SELECT        = 0x3F,
  BEO_CMD_SPEAKER       = 0x44,
  BEO_CMD_PICTURE       = 0x45,
  BEO_CMD_TURN          = 0x46,
  BEO_CMD_LOUDNESS      = 0x48,
  BEO_CMD_BASS          = 0x4D,
  BEO_CMD_TREBLE        = 0x4E,
  BEO_CMD_BALANCE       = 0x4F,
  BEO_CMD_LIST          = 0x58,
  BEO_CMD_MENU          = 0x5C,
  BEO_CMD_VOLUME_UP     = 0x60,
  BEO_CMD_VOLUME_DOWN   = 0x64,
  BEO_CMD_LEFT_REPEAT   = 0x70,
  BEO_CMD_RIGHT_REPEAT  = 0x71,
  BEO_CMD_UP_REPEAT     = 0x72,
  BEO_CMD_DOWN_REPEAT   = 0x73,
  BEO_CMD_GO_REPEAT     = 0x75,
  BEO_CMD_GREEN_REPEAT  = 0x76,
  BEO_CMD_YELLOW_REPEAT = 0x77,
  BEO_CMD_BLUE_REPEAT   = 0x78,
  BEO_CMD_RED_REPEAT    = 0x79,
  BEO_CMD_EXIT          = 0x7F,
  BEO_CMD_TV            = 0x80,
  BEO_CMD_RADIO         = 0x81,
  BEO_CMD_VIDEO_AUX     = 0x82,
  BEO_CMD_AUDIO_AUX     = 0x83,
  BEO_CMD_VIDEO_TAPE    = 0x85,
  BEO_CMD_DVD           = 0x86,
  BEO_CMD_CAMCORD       = 0x87,
  BEO_CMD_TEXT          = 0x88,
  BEO_CMD_SP_DEMO      = 0x89,
  BEO_CMD_DIGITAL_TV   = 0x8A,
  BEO_CMD_PC           = 0x8B,
  BEO_CMD_DOOR_CAM     = 0x8D,
  BEO_CMD_AUDIO_TAPE   = 0x91,
  BEO_CMD_CD           = 0x92,
  BEO_CMD_PHONO        = 0x93,
  BEO_CMD_AUDIO_TAPE_2 = 0x94,
  BEO_CMD_CD2          = 0x97,
  BEO_CMD_LIGHT        = 0x9B,
  BEO_CMD_AV           = 0xBF,
  BEO_CMD_YELLOW       = 0xD4,
  BEO_CMD_GREEN        = 0xD5,
  BEO_CMD_BLUE         = 0xD8,
  BEO_CMD_RED          = 0xD9,
  // Navigation (POV hat on newer Beo4 remotes, address 0xA1)
  BEO_CMD_NAV_GO       = 0x13,
  BEO_CMD_BACK         = 0x14,
  BEO_CMD_NAV_UP       = 0xCA,
  BEO_CMD_NAV_DOWN     = 0xCB,
  BEO_CMD_NAV_LEFT     = 0xCC,
  BEO_CMD_NAV_RIGHT    = 0xCD,
  // Custom codes for IR eye buttons (not part of Beo4 protocol)
  BEO_CMD_EYE_TIMER    = 0xF0,
};

inline bool is_valid_beo_address(uint8_t address) {
  switch (address) {
    case BEO_ADDR_VIDEO:
    case BEO_ADDR_AUDIO:
    case BEO_ADDR_VIDEOTAPE:
    case BEO_ADDR_ALL:
    case BEO_ADDR_LIGHT:
    case BEO_ADDR_SPDEMO:
    case BEO_ADDR_NAV:
      return true;
    default:
      return false;
  }
}

inline const char *beo_address_name(uint8_t address) {
  switch (address) {
    case BEO_ADDR_VIDEO:     return "VIDEO";
    case BEO_ADDR_AUDIO:     return "AUDIO";
    case BEO_ADDR_VIDEOTAPE: return "VIDEOTAPE";
    case BEO_ADDR_ALL:       return "ALL";
    case BEO_ADDR_LIGHT:     return "LIGHT";
    case BEO_ADDR_SPDEMO:    return "SPDEMO";
    case BEO_ADDR_NAV:       return "NAV";
    default:                 return "UNKNOWN";
  }
}

inline const char *beo_command_name(uint8_t command) {
  switch (command) {
    case BEO_CMD_NUMBER_0:      return "NUMBER_0";
    case BEO_CMD_NUMBER_1:      return "NUMBER_1";
    case BEO_CMD_NUMBER_2:      return "NUMBER_2";
    case BEO_CMD_NUMBER_3:      return "NUMBER_3";
    case BEO_CMD_NUMBER_4:      return "NUMBER_4";
    case BEO_CMD_NUMBER_5:      return "NUMBER_5";
    case BEO_CMD_NUMBER_6:      return "NUMBER_6";
    case BEO_CMD_NUMBER_7:      return "NUMBER_7";
    case BEO_CMD_NUMBER_8:      return "NUMBER_8";
    case BEO_CMD_NUMBER_9:      return "NUMBER_9";
    case BEO_CMD_CLEAR:         return "CLEAR";
    case BEO_CMD_STORE:         return "STORE";
    case BEO_CMD_STANDBY:       return "STANDBY";
    case BEO_CMD_MUTE:          return "MUTE";
    case BEO_CMD_INDEX:         return "INDEX";
    case BEO_CMD_UP:            return "UP";
    case BEO_CMD_DOWN:          return "DOWN";
    case BEO_CMD_TUNE:          return "TUNE";
    case BEO_CMD_CLOCK:         return "CLOCK";
    case BEO_CMD_FORMAT:        return "FORMAT";
    case BEO_CMD_LEFT:          return "LEFT";
    case BEO_CMD_RETURN:        return "RETURN";
    case BEO_CMD_RIGHT:         return "RIGHT";
    case BEO_CMD_GO:            return "GO";
    case BEO_CMD_STOP:          return "STOP";
    case BEO_CMD_RECORD:        return "RECORD";
    case BEO_CMD_SELECT:        return "SELECT";
    case BEO_CMD_SPEAKER:       return "SPEAKER";
    case BEO_CMD_PICTURE:       return "PICTURE";
    case BEO_CMD_TURN:          return "TURN";
    case BEO_CMD_LOUDNESS:      return "LOUDNESS";
    case BEO_CMD_BASS:          return "BASS";
    case BEO_CMD_TREBLE:        return "TREBLE";
    case BEO_CMD_BALANCE:       return "BALANCE";
    case BEO_CMD_LIST:          return "LIST";
    case BEO_CMD_MENU:          return "MENU";
    case BEO_CMD_VOLUME_UP:     return "VOLUME_UP";
    case BEO_CMD_VOLUME_DOWN:   return "VOLUME_DOWN";
    case BEO_CMD_LEFT_REPEAT:   return "LEFT_REPEAT";
    case BEO_CMD_RIGHT_REPEAT:  return "RIGHT_REPEAT";
    case BEO_CMD_UP_REPEAT:     return "UP_REPEAT";
    case BEO_CMD_DOWN_REPEAT:   return "DOWN_REPEAT";
    case BEO_CMD_GO_REPEAT:     return "GO_REPEAT";
    case BEO_CMD_GREEN_REPEAT:  return "GREEN_REPEAT";
    case BEO_CMD_YELLOW_REPEAT: return "YELLOW_REPEAT";
    case BEO_CMD_BLUE_REPEAT:   return "BLUE_REPEAT";
    case BEO_CMD_RED_REPEAT:    return "RED_REPEAT";
    case BEO_CMD_EXIT:          return "EXIT";
    case BEO_CMD_TV:            return "TV";
    case BEO_CMD_RADIO:         return "RADIO";
    case BEO_CMD_VIDEO_AUX:     return "VIDEO_AUX";
    case BEO_CMD_AUDIO_AUX:     return "AUDIO_AUX";
    case BEO_CMD_VIDEO_TAPE:    return "VIDEO_TAPE";
    case BEO_CMD_DVD:           return "DVD";
    case BEO_CMD_CAMCORD:       return "CAMCORD";
    case BEO_CMD_TEXT:          return "TEXT";
    case BEO_CMD_SP_DEMO:      return "SP_DEMO";
    case BEO_CMD_DIGITAL_TV:   return "DIGITAL_TV";
    case BEO_CMD_PC:           return "PC";
    case BEO_CMD_DOOR_CAM:     return "DOOR_CAM";
    case BEO_CMD_AUDIO_TAPE:   return "AUDIO_TAPE";
    case BEO_CMD_CD:           return "CD";
    case BEO_CMD_PHONO:        return "PHONO";
    case BEO_CMD_AUDIO_TAPE_2: return "AUDIO_TAPE_2";
    case BEO_CMD_CD2:          return "CD2";
    case BEO_CMD_LIGHT:        return "LIGHT";
    case BEO_CMD_AV:           return "AV";
    case BEO_CMD_YELLOW:       return "YELLOW";
    case BEO_CMD_GREEN:        return "GREEN";
    case BEO_CMD_BLUE:         return "BLUE";
    case BEO_CMD_RED:          return "RED";
    case BEO_CMD_NAV_GO:       return "NAV_GO";
    case BEO_CMD_BACK:         return "BACK";
    case BEO_CMD_NAV_UP:       return "NAV_UP";
    case BEO_CMD_NAV_DOWN:     return "NAV_DOWN";
    case BEO_CMD_NAV_LEFT:     return "NAV_LEFT";
    case BEO_CMD_NAV_RIGHT:    return "NAV_RIGHT";
    case BEO_CMD_EYE_TIMER:    return "EYE_TIMER";
    default:                   return "UNKNOWN";
  }
}
