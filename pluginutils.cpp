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

#include <cstring>
#include "pluginutils.h"


extern void *(*logprintf)(const char *fmt, ...);

namespace pluginutils
{

	bool GetPublicVariable(AMX *amx, const char *name, cell &value)
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

	void SplitVersion(cell version, int &major, int &minor, int &build)
	{
		major = ((int)version >> 24) & 0xFF;
		minor = ((int)version >> 16) & 0xFF;
		build = (int)version & 0xFFFF;
	}

	bool CheckIncludeVersion(AMX *amx)
	{
		cell include_version;
		bool include_version_found = GetPublicVariable(amx, INCLUDE_VERSION_VAR_NAME, include_version);
		if (!include_version_found || include_version == PLUGIN_VERSION)
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
		const cell num_natives =
			(cell)(hdr->libraries - hdr->natives) / (cell)defsize;
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
			const int flags_bck = amx->flags;
			const cell cip_bck = amx->cip;
			const cell pri_bck = amx->pri;
	#if defined AMX_FLAG_BROWSE
			amx->flags |= AMX_FLAG_BROWSE;
	#else
			amx->flags |= AMX_FLAG_VERIFY;
	#endif
			amx->pri = 0;
			amx->cip = 0;
			amx_Exec(amx, (cell *)(size_t)&jump_table, AMX_EXEC_CONT);
			amx->cip = cip_bck;
			amx->pri = pri_bck;
			amx->flags = flags_bck;
#endif // CUR_FILE_VERSION < 11
			jump_table_checked = true;
		}

#ifdef AMX_FLAG_SYSREQN
		if (amx->flags & AMX_FLAG_SYSREQN)
		{
			op_addr = amx->cip - 3 * sizeof(cell);
			if (op_addr < 0)
				goto ret;
			opcode = *(cell *)(code + (size_t)op_addr);
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
		opcode = *(cell *)(void *)(code + (size_t)op_addr);

		if ((jump_table != NULL)
			? (opcode == jump_table[OP_SYSREQ_C]) : (opcode == OP_SYSREQ_C))
		{
#ifdef AMX_FLAG_SYSREQN
	sysreq_c:
#endif
			const cell func_index =
				*(cell *)(void *)(code + (size_t)op_addr + sizeof(cell));
			if (func_index < num_natives)
				func = (AMX_FUNCSTUB *)((unsigned char *)(void *)natives +
					(size_t)func_index * (size_t)hdr->defsize);
			goto ret;
		}
		if ((jump_table != NULL)
			? (opcode == jump_table[OP_SYSREQ_D]) : (opcode == OP_SYSREQ_D))
		{
#ifdef AMX_FLAG_SYSREQN
	sysreq_d:
#endif
			const ucell func_addr =
				*(ucell *)(void *)(code + (size_t)op_addr + sizeof(cell));
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
		if (hdr->defsize == (int16_t)sizeof(AMX_FUNCSTUB))
			return (const char *)func->name;
		return (const char *)
			((size_t)hdr + (size_t)((AMX_FUNCSTUBNT *)func)->nameofs);
#else
		return (const char *)((size_t)hdr + (size_t)func->nameofs);
#endif
	}

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

	bool ReplaceNative(AMX *amx, const char *name, AMX_NATIVE ntv, AMX_NATIVE *orig)
	{
		AMX_HEADER *hdr = (AMX_HEADER *)amx->base;
		unsigned char *natives = (unsigned char *)hdr + (size_t)hdr->natives;
		unsigned char *libraries = (unsigned char *)hdr + (size_t)hdr->libraries;
		const size_t defsize = (size_t)hdr->defsize;
		AMX_FUNCSTUB *func = (AMX_FUNCSTUB *)natives;
		AMX_FUNCSTUB *end = (AMX_FUNCSTUB *)libraries;
		while (func < end)
		{
			const char *ntv_name = (defsize == sizeof(AMX_FUNCSTUB))
				? (const char*)func->name
				: (const char *)((size_t)hdr + (size_t)((AMX_FUNCSTUBNT *)func)->nameofs);
			if (strcmp(name, ntv_name) != 0)
			{
				func = (AMX_FUNCSTUB *)((unsigned char *)func + defsize);
				continue;
			}
			if (orig != NULL)
				*orig = (AMX_NATIVE)(size_t)func->address;
			func->address = (ucell)(size_t)ntv;
			return true;
		}
		return false;
	}

	char *GetCString(AMX *amx, cell address, int &error)
	{
		int len;
		cell *cptr;
		char *cstr;

		error = amx_GetAddr(amx, address, &cptr);
		if (error != AMX_ERR_NONE)
			return NULL;

		error = amx_StrLen(cptr, &len);
		if (error != AMX_ERR_NONE)
			return NULL;

		cstr = (char *)malloc((size_t)(len + 1) * sizeof(char));
		if (cstr == NULL)
		{
			error = AMX_ERR_MEMORY;
			return NULL;
		}

		error = amx_GetString(cstr, cptr, 0, (size_t)(len + 1));
		if (error != AMX_ERR_NONE)
			return NULL;

		return cstr;
	}

	std::string GetCXXString(AMX *amx, cell address, int &error)
	{
		int len;
		cell *cptr;
		char *cstr;

		error = amx_GetAddr(amx, address, &cptr);
		if (error != AMX_ERR_NONE)
			return NULL;

		error = amx_StrLen(cptr, &len);
		if (error != AMX_ERR_NONE)
			return NULL;

		cstr = (char *)alloca((size_t)(len + 1) * sizeof(char));
		if (cstr == NULL)
		{
			error = AMX_ERR_MEMORY;
			return NULL;
		}

		error = amx_GetString(cstr, cptr, 0, (size_t)(len + 1));
		if (error != AMX_ERR_NONE)
			return NULL;

		std::string str(cstr);
		return str;
	}

	bool SetCString(AMX *amx, cell address, cell size, const char *str, bool pack)
	{
		cell *cptr;
		int error;

		error = amx_GetAddr(amx, address, &cptr);
		if (error != AMX_ERR_NONE)
			return false;

		return (amx_SetString(cptr, str, (pack) ? 1 : 0, 0, size) != AMX_ERR_NONE);
	}

	bool SetCXXString(AMX *amx, cell address, cell size, const std::string &str, bool pack)
	{
		return SetCString(amx, address, size, str.c_str(), pack);
	}

}
