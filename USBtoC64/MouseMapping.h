#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <string.h>

#include "hid_usage_mouse.h"

#ifndef MOUSE_MAP_CUSTOM
  // 0 = BOOT protocol (no wheel), 1 = REPORT protocol with custom mapping table.
  #define MOUSE_MAP_CUSTOM 0
#endif

enum MM_AxisFormat : uint8_t {
  MM_AXIS_I8 = 0,
  MM_AXIS_I12,
  MM_AXIS_I16
};

struct MM_ReportMap {
  uint8_t length;
  uint8_t buttons_index;
  uint8_t x_index;
  uint8_t y_index;
  uint8_t wheel_index; // 0xFF = no wheel
  MM_AxisFormat format;
};

#if (MOUSE_MAP_CUSTOM == 1)
// Edit this table to match your mouse report layout(s).
static const MM_ReportMap MM_REPORT_MAPS[] = {
  // len, btn, x, y, wheel, format
  { 3, 0, 1, 2, 0xFF, MM_AXIS_I8  }, // boot mouse (no wheel)
  { 4, 0, 1, 2, 3,    MM_AXIS_I8  }, // boot mouse + wheel
  { 5, 0, 1, 2, 4,    MM_AXIS_I12 }, // 12-bit packed XY + wheel
  { 6, 1, 2, 3, 5,    MM_AXIS_I12 }, // report ID + 12-bit packed XY + wheel
  { 8, 1, 2, 4, 6,    MM_AXIS_I16 } 
};
static const size_t MM_REPORT_MAPS_COUNT = sizeof(MM_REPORT_MAPS) / sizeof(MM_REPORT_MAPS[0]);
#endif

static inline int16_t MM_SignExtend12(uint16_t v) {
  v &= 0x0FFF;
  if (v & 0x0800) return (int16_t)(v | 0xF000);
  return (int16_t)v;
}

static inline int8_t MM_ClampToI8(int16_t v) {
  if (v > 127) return 127;
  if (v < -127) return -127;
  return (int8_t)v;
}

#if (MOUSE_MAP_CUSTOM == 1)
static inline const MM_ReportMap *MM_FindMap(int length) {
  for (size_t i = 0; i < MM_REPORT_MAPS_COUNT; i++) {
    if (MM_REPORT_MAPS[i].length == (uint8_t)length) return &MM_REPORT_MAPS[i];
  }
  return NULL;
}
#endif

static inline void MM_DecodeMouseReport(const uint8_t *data, int length,
                                        hid_mouse_input_report_boot_t *out,
                                        int8_t *wheel) {
  if (out) memset(out, 0, sizeof(*out));
  if (wheel) *wheel = 0;

#if (MOUSE_MAP_CUSTOM == 0)
  if (!out || length < 3) return;
  memcpy(&out->buttons, &data[0], 1);
  out->x_displacement = (int8_t)data[1];
  out->y_displacement = (int8_t)data[2];
  return;
#else
  const MM_ReportMap *map = MM_FindMap(length);
  if (!map || !out) return;

  if (map->buttons_index < (uint8_t)length) {
    memcpy(&out->buttons, &data[map->buttons_index], 1);
  }

  switch (map->format) {
    case MM_AXIS_I8:
      if (map->x_index < (uint8_t)length) out->x_displacement = (int8_t)data[map->x_index];
      if (map->y_index < (uint8_t)length) out->y_displacement = (int8_t)data[map->y_index];
      break;
    case MM_AXIS_I12: {
      // 12-bit packed uses bytes: b1 = x_index, b2 = y_index, b3 = y_index + 1.
      uint8_t b3_index = (uint8_t)(map->y_index + 1);
      if (map->x_index < (uint8_t)length &&
          map->y_index < (uint8_t)length &&
          b3_index < (uint8_t)length) {
        uint8_t b1 = data[map->x_index];
        uint8_t b2 = data[map->y_index];
        uint8_t b3 = data[b3_index];
        int16_t x12 = MM_SignExtend12((uint16_t)b1 | ((uint16_t)(b2 & 0x0F) << 8));
        int16_t y12 = MM_SignExtend12((uint16_t)((b2 >> 4) & 0x0F) | ((uint16_t)b3 << 4));
        out->x_displacement = MM_ClampToI8(x12);
        out->y_displacement = MM_ClampToI8(y12);
      }
      break;
    }
    case MM_AXIS_I16:
      if ((uint8_t)(map->x_index + 1) < (uint8_t)length &&
          (uint8_t)(map->y_index + 1) < (uint8_t)length) {
        uint8_t b1 = data[map->x_index];
        uint8_t b2 = data[map->x_index + 1];
        uint8_t b3 = data[map->y_index];
        uint8_t b4 = data[map->y_index + 1];
        int16_t x16 = (int16_t)((uint16_t)b1 | ((uint16_t)b2 << 8));
        int16_t y16 = (int16_t)((uint16_t)b3 | ((uint16_t)b4 << 8));
        out->x_displacement = MM_ClampToI8(x16);
        out->y_displacement = MM_ClampToI8(y16);
      }
      break;
  }

  if (wheel && map->wheel_index != 0xFF && map->wheel_index < (uint8_t)length) {
    *wheel = (int8_t)data[map->wheel_index];
  }
#endif
}
