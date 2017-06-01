/*==============================================================================
	Copyright (c) 2014-2017 Stanislav Gromov.

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the
use of this software.

Permission is granted to anyone to use this software for
any purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software in
	a product, an acknowledgment in the product documentation would be
	appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.

3.	This notice may not be removed or altered from any source distribution.
==============================================================================*/

#ifndef _PLUGINUTILS_H
#define _PLUGINUTILS_H

#include <stdlib.h>
#include "SDK/amx/amx.h"
#include "pluginconfig.h"

#if !defined FORCE_INLINE
	#if defined _MSC_VER
		#define FORCE_INLINE __forceinline
	#else
		#define FORCE_INLINE __attribute__((always_inline)) inline
	#endif
#endif
#if !defined REGISTER_VAR
	#if defined __GNUC__
		#define REGISTER_VAR register
	#else
		#define REGISTER_VAR
	#endif
#endif

/*
	Remove the #if block below and include osdefs.h if you want to use functions
	AlignCell, AlignCellArray, CopyAndAlignCellArray and GetPackedArrayCharAddr
	in something other than a SA-MP plugin.
*/
#ifndef BYTE_ORDER
	#define LITTLE_ENDIAN 1234
	#define BIG_ENDIAN 4321
	#define BYTE_ORDER LITTLE_ENDIAN
#endif


extern void *(*logprintf)(const char *fmt, ...);

namespace pluginutils
{

	/*
		Retrieves the value of a public variable.
	*/
	inline bool GetPublicVariable(AMX *amx, const char *name, cell &value)
	{
		cell amx_addr;
		value = 0;
		if (amx_FindPubVar(amx, name, &amx_addr) != AMX_ERR_NONE)
			return false;
		cell *phys_addr;
		if (amx_GetAddr(amx, amx_addr, &phys_addr) != AMX_ERR_NONE)
			return false;
		value = *phys_addr;
		return true;
	}

	/*
		Splits plugin/include version into major, minor and build number.
	*/
	inline void SplitVersion(cell version, int &major, int &minor, int &build)
	{
		major = ((int)version >> 24) & 0xFF;
		minor = ((int)version >> 16) & 0xFF;
		build = (int)version & 0xFFFF;
	}

	/*
		Checks if the include and plugin versions are equal.
		Prints an error message if versions don't match.
	*/
	bool CheckIncludeVersion(AMX *amx)
	{
		cell include_version;
		bool include_version_found = GetPublicVariable(amx, INCLUDE_VERSION_VAR_NAME, include_version);
		if (include_version_found && include_version == PLUGIN_VERSION)
			return true;
		int inc_ver_major, inc_ver_minor, inc_ver_build, plug_ver_major, plug_ver_minor, plug_ver_build;
		pluginutils::SplitVersion(include_version, inc_ver_major, inc_ver_minor, inc_ver_build);
		pluginutils::SplitVersion(PLUGIN_VERSION, plug_ver_major, plug_ver_minor, plug_ver_build);
		logprintf(
			"%s: Include file version (%d.%d.%d) does not match with the plugin version (%d.%d.%d)!",
			PLUGIN_NAME, inc_ver_major, inc_ver_minor, inc_ver_build,
			plug_ver_major, plug_ver_minor, plug_ver_build
		);
		logprintf("%s: Please recompile your script with the latest version of include file.", PLUGIN_NAME);
		return false;
	}

	/*
		Swaps bytes in a cell on Little Endian architectures.
	*/
	FORCE_INLINE cell AlignCell(cell value)
	{
#if BYTE_ORDER == LITTLE_ENDIAN
#if defined _MSC_VER
		if (sizeof(cell) == sizeof(unsigned short))
			value = (cell)_byteswap_ushort((unsigned short)value);
		else if (sizeof(cell) == sizeof(unsigned long))
			value = (cell)_byteswap_ulong((unsigned long)value);
		else if (sizeof(cell) == sizeof(unsigned __int64))
			value = (cell)_byteswap_uint64((unsigned __int64)value);
#elif (defined __GNUC__ || defined __clang__) && (PAWN_CELL_SIZE != 16)
	#if PAWN_CELL_SIZE == 32
		value = (cell)__builtin_bswap32((uint32_t)value);
	#elif PAWN_CELL_SIZE == 64
		value = (cell)__builtin_bswap64((uint64_t)value);
	#endif
#else
		/*
			This code could be simplified by using a loop, but some compilers
			don't unroll loops while unneeded #if branches are guaranteed
			to be optimized out - that's just how the C/C++ preprocessor works.
		*/
		unsigned char *bytes = (unsigned char *)(size_t)(&value);
		unsigned char t;
	#if PAWN_CELL_SIZE >= 16
		t = bytes[0], bytes[0] = bytes[sizeof(cell) - 1], bytes[sizeof(cell) - 1] = t;
	#if PAWN_CELL_SIZE >= 32
		t = bytes[1], bytes[1] = bytes[sizeof(cell) - 2], bytes[sizeof(cell) - 2] = t;
	#if PAWN_CELL_SIZE == 64
		t = bytes[2], bytes[2] = bytes[sizeof(cell) - 3], bytes[sizeof(cell) - 3] = t;
		t = bytes[3], bytes[3] = bytes[sizeof(cell) - 4], bytes[sizeof(cell) - 4] = t;
	#endif // PAWN_CELL_SIZE == 64
	#endif // PAWN_CELL_SIZE == 32
	#endif // PAWN_CELL_SIZE == 16
#endif
#endif // BYTE_ORDER == LITTLE_ENDIAN
		return value;
	}

	/*
		Swaps bytes in array of cells on Little Endian architectures.
	*/
	FORCE_INLINE void AlignCellArray(cell a[], size_t num_elements)
	{
#if BYTE_ORDER==LITTLE_ENDIAN
		REGISTER_VAR cell *ptr = &a[0];
		REGISTER_VAR const cell *end = &a[num_elements];
		const size_t num_remaining = (num_elements % 4);
		REGISTER_VAR const cell *aligned_end = end - num_remaining;
		while (ptr < aligned_end)
		{
			AlignCell(*ptr);
			AlignCell(*(ptr + 1));
			AlignCell(*(ptr + 2));
			AlignCell(*(ptr + 3));
			ptr += 4;
		}
		switch (num_remaining)
		{
		case 3:
			AlignCell(*(end - 3));
			// Fallthrough.
		case 2:
			AlignCell(*(end - 2));
			// Fallthrough.
		case 1:
			AlignCell(*(end - 1));
		}
#endif // BYTE_ORDER == LITTLE_ENDIAN
	}

	/*
		Copies an array of cells.
		Swaps cell bytes on the fly on Little Endian architectures.
	*/
	FORCE_INLINE void CopyAndAlignCellArray(cell *dest, cell *src, size_t num_cells)
	{
#if BYTE_ORDER == LITTLE_ENDIAN
		const cell *dest_end_ptr = &dest[num_cells];
		for ( ; dest < dest_end_ptr; )
			*(dest++) = AlignCell(*(src++));
#else
		memcpy(dest, src, num_cells * sizeof(cell));
#endif
	}

	/*
		Returns the address of a byte in packed array.
	*/
	inline unsigned char *GetPackedArrayCharAddr(cell arr[], cell index)
	{
		unsigned char *addr;
#if BYTE_ORDER == LITTLE_ENDIAN
		const size_t idx_mod_cellsize = (size_t)index & (sizeof(cell) - 1);
		addr = &((unsigned char *)arr)[(size_t)index - idx_mod_cellsize] +
			(sizeof(cell) - 1) - idx_mod_cellsize;
#else // BYTE_ORDER == LITTLE_ENDIAN
		addr = &((unsigned char *)arr)[(size_t)index];
#endif // BYTE_ORDER == LITTLE_ENDIAN
		return addr;
	}

	/*
		Returns the name of the current native function.
	*/
	const char *GetCurrentNativeFunctionName(AMX *amx)
	{ // http://pro-pawn.ru/showthread.php?14522
#if (6 <= CUR_FILE_VERSION) && (CUR_FILE_VERSION <= 8)
		const cell OP_SYSREQ_C = 123, OP_SYSREQ_D = 135;
#elif CUR_FILE_VERSION == 9
		const cell OP_SYSREQ_C = 123, OP_SYSREQ_D = 158, OP_SYSREQ_ND = 159;
		const cell OP_SYSREQ_N = 135;
#elif CUR_FILE_VERSION == 10
		const cell OP_SYSREQ_C = 123, OP_SYSREQ_D = 213, OP_SYSREQ_ND = 214;
		const cell OP_SYSREQ_N = 135;
#elif CUR_FILE_VERSION == 11
		const cell OP_SYSREQ_C = 69, OP_SYSREQ_D = 75, OP_SYSREQ_ND = 76;
		const cell OP_SYSREQ_N = 112;
#else
		#error Unsupported version of AMX instruction set.
#endif

		AMX_HEADER *hdr = (AMX_HEADER *)amx->base;
		AMX_FUNCSTUB *natives =
			(AMX_FUNCSTUB *)((size_t)hdr + (size_t)hdr->natives);
		const size_t defsize = (size_t)hdr->defsize;
		const size_t num_natives =
			(size_t)(hdr->libraries - hdr->natives) / defsize;
		AMX_FUNCSTUB *func = NULL;
#ifndef AMX_FLAG_OVERLAY
		unsigned char *code = amx->base + (size_t)(hdr->cod);
#else
		unsigned char *code = amx->code;
#endif
		cell op_addr, opcode;

		static cell *jump_table = NULL;
		static bool jump_table_checked = false;
		if (!jump_table_checked)
		{
			// On Pawn 4.0 there's no clear way to get the jump table,
			// so only the ANSI C version of the interpreter core is supported.
#if CUR_FILE_VERSION < 11
			// Set the AMX_FLAG_BROWSE flag and call amx_Exec.
			// If there's no jump table (ANSI C version) amx_Exec would just
			// stumble upon the HALT instruction at address 0 and return.
			const int flags = amx->flags;
			const cell cip = amx->cip;
	#if defined AMX_FLAG_BROWSE
			amx->flags |= AMX_FLAG_BROWSE;
	#else
			amx->flags |= AMX_FLAG_VERIFY;
	#endif
			amx->cip = 0;
			amx_Exec(amx, (cell *)(size_t)&jump_table, AMX_EXEC_CONT);
			amx->cip = cip;
			amx->flags = flags;
#endif // CUR_FILE_VERSION < 11
			jump_table_checked = true;
		}

#ifdef AMX_FLAG_SYSREQN
		if (amx->flags & AMX_FLAG_SYSREQN)
		{
			op_addr = amx->cip - 3 * sizeof(cell);
			if (op_addr < 0)
				goto ret;
			opcode = *(cell *)&(code[(size_t)op_addr]);
			if (jump_table != NULL)
			{
				if (opcode == jump_table[OP_SYSREQ_N])
					goto sysreq_c;
				if (opcode == jump_table[OP_SYSREQ_ND])
					goto sysreq_d;
				goto ret;
			}
			if (opcode == OP_SYSREQ_N)
				goto sysreq_c;
			if (opcode == OP_SYSREQ_ND)
				goto sysreq_d;
			goto ret;
		}
#endif

		op_addr = amx->cip - 2 * (cell)sizeof(cell);
		if (op_addr < 0)
			goto ret;
		opcode = *(cell *)&(code[(size_t)op_addr]);

		if ((jump_table != NULL)
			? (opcode == jump_table[OP_SYSREQ_C]) : (opcode == OP_SYSREQ_C))
		{
#ifdef AMX_FLAG_SYSREQN
	sysreq_c:
#endif
			const size_t func_index =
				(size_t)*(cell *)&(code[(size_t)op_addr + sizeof(cell)]);
			if (func_index >= num_natives)
				func = &natives[func_index];
			goto ret;
		}
		if ((jump_table != NULL)
			? (opcode == jump_table[OP_SYSREQ_D]) : (opcode == OP_SYSREQ_D))
		{
#ifdef AMX_FLAG_SYSREQN
	sysreq_d:
#endif
			const ucell func_addr =
				*(ucell *)&(code[(size_t)op_addr + sizeof(cell)]);
			func = natives;
			size_t libraries = (size_t)amx->base + (size_t)hdr->libraries;
			for (; (size_t)natives < libraries; *((size_t *)&func) += defsize)
				if (func->address == func_addr)
					goto ret;
			func = NULL;
			goto ret;
		}

	ret:
		static const char str_unknown[] = "(unknown)";
		if (NULL == func)
			return str_unknown;
#if CUR_FILE_VERSION < 11
		if (hdr->defsize != (int16_t)sizeof(AMX_FUNCSTUBNT))
			return (const char *)func->name;
		return (const char *)
			((size_t)hdr + (size_t)((AMX_FUNCSTUBNT *)func)->nameofs);
#else
		return (const char *)((size_t)hdr + (size_t)func->nameofs);
#endif
	}

	/*
		Checks the number of arguments.
		Prints an error message if the actual number of arguments is less than expected.
	*/
	bool CheckNumberOfArguments(AMX *amx, const cell *params, int num_expected)
	{
		if (((int)params[0] / (int)sizeof(cell)) >= num_expected)
			return true;
		amx_RaiseError(amx, AMX_ERR_PARAMS);
		logprintf(
			"%s:%s: Incorrect number of arguments (expected %d, got %d).",
			PLUGIN_NAME, GetCurrentNativeFunctionName(amx), num_expected, (int)params[0]
		);
		return false;
	}

};


#endif // _PLUGINUTILS_H
