// license:BSD-3-Clause
// copyright-holders:
// minimal stand-in for MAME's emu.h, enough for i960dis.cpp
#include "disasmintf.h"
#include <cstdint>
#include <ostream>
#include "strformat.h"
#include "coretmpl.h"
using u8 = uint8_t; using u16 = uint16_t; using u32 = uint32_t; using u64 = uint64_t;
using s8 = int8_t; using s16 = int16_t; using s32 = int32_t; using offs_t = uint32_t;
