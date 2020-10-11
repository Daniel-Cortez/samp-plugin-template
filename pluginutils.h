/*==============================================================================
	Copyright (c) 2014-2018 Stanislav Gromov.

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

#include <cstdlib>
#include <string>
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


namespace pluginutils
{

	/*
		Retrieves the value of a public variable.
	*/
	bool GetPublicVariable(AMX *amx, const char *name, cell &value);

	/*
		Splits plugin/include version into major, minor and build number.
	*/
	void SplitVersion(cell version, int &major, int &minor, int &build);

	/*
		Checks if the include and plugin versions are equal.
		Prints an error message if versions don't match.
	*/
	bool CheckIncludeVersion(AMX *amx);

	/*
		Swaps bytes in a cell on little-endian architectures.
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
		Swaps bytes in array of cells on little-endian architectures.
	*/
	FORCE_INLINE void AlignCellArray(cell a[], size_t num_elements)
	{
#if BYTE_ORDER == LITTLE_ENDIAN
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
		Swaps cell bytes on the fly on little-endian architectures.
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
		Returns the address of a byte in a packed array.
	*/
	FORCE_INLINE unsigned char *GetPackedArrayCharAddr(cell arr[], cell index)
	{
#if BYTE_ORDER == LITTLE_ENDIAN
		const size_t idx_mod_cellsize = (size_t)index & (sizeof(cell) - 1);
		return (unsigned char *)(size_t)arr + (size_t)index - idx_mod_cellsize +
			(sizeof(cell) - 1) - idx_mod_cellsize;
#else // BYTE_ORDER == LITTLE_ENDIAN
		return &((unsigned char *)arr)[(ucell)index];
#endif // BYTE_ORDER == LITTLE_ENDIAN
	}

	/*
		Returns the name of the current native function.
	*/
	const char *GetCurrentNativeFunctionName(AMX *amx);

	/*
		Checks the number of arguments.
		Prints an error message if the actual number of arguments is less than expected.
	*/
	bool CheckNumberOfArguments(AMX *amx, const cell *params, int num_expected);

	/*
		Replaces one native function with another.
	*/
	bool ReplaceNative(AMX *amx, const char *name, AMX_NATIVE ntv, AMX_NATIVE *orig);

	/*
		Obtains a NUL-terminated string, or returns NULL if the string address is invalid.
		NOTE: The storage is allocated with 'malloc()' so in order to avoid memory leaks
		the pointer must be passed to 'free()' when it's not needed anymore.
	*/
	char *GetCString(AMX *amx, cell address, int &error);

	/*
		Obtains a C++ string or returns an empty string if the address is invalid.
	*/
	std::string GetCXXString(AMX *amx, cell address, int &error);

	/*
		Sets a string in script memory from either a C string (NUL-terminated)
		or a C++ string (std::string).
	*/
	bool SetCString(AMX *amx, cell address, cell size, const char *str, bool pack = false);
	bool SetCXXString(AMX *amx, cell address, cell size, const std::string &str, bool pack = false);

}


#endif // _PLUGINUTILS_H
