/*
	TODO: Put your copyright notice and license text here.
*/


#include <cstddef>

#include "SDK/amx/amx.h"
#include "SDK/plugincommon.h"
#include "pluginconfig.h"
#include "pluginutils.h"

#define CheckArgs() pluginutils::CheckNumberOfArguments(amx, params, num_args_expected)


extern void *pAMXFunctions;
void *(*logprintf)(const char *fmt, ...);


static cell AMX_NATIVE_CALL n_HelloWorld(AMX *amx, cell *params)
{
	logprintf("%s: This line was printed from a plugin", pluginutils::GetCurrentNativeFunctionName(amx));
	return 1;
}

static cell AMX_NATIVE_CALL n_HelloWorld_PrintNumber(AMX *amx, cell *params)
{
	enum
	{
		args_size,
		arg_number,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckArgs())
		return 0;
	logprintf("%s: %d", pluginutils::GetCurrentNativeFunctionName(amx), params[arg_number]);
	return 1;
}

static cell AMX_NATIVE_CALL n_HelloWorld_PrintString(AMX *amx, cell *params)
{
	enum
	{
		args_size,
		arg_str,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckArgs())
		return 0;

	char *str;
	int error;

	str = pluginutils::GetCString(amx, params[arg_str], error);
	if (error != AMX_ERR_NONE)
		return amx_RaiseError(amx, error), 0;
	logprintf("%s: %s", pluginutils::GetCurrentNativeFunctionName(amx), str);

	free(str); // The string returned by GetCString() needs to be freed manually.
	return 1;
}

static cell AMX_NATIVE_CALL n_HelloWorld_CheckArgsTest(AMX *amx, cell *params)
{
	enum
	{
		args_size,
		arg_first,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckArgs())
		return 0;
	// The number of arguments for this function defined in the include file
	// is invalid, so this code should never occur.
	logprintf("This line shouldn't be printed");
	return 1;
}

static AMX_NATIVE orig_IsPlayerConnected;
static cell AMX_NATIVE_CALL hook_IsPlayerConnected(AMX *amx, cell *params)
{
	logprintf("Hello from hook_IsPlayerConnected");
	return orig_IsPlayerConnected(amx, params);
}


static AMX_NATIVE_INFO plugin_natives[] =
{
	{ "HelloWorld", n_HelloWorld },
	{ "HelloWorld_PrintNumber", n_HelloWorld_PrintNumber },
	{ "HelloWorld_PrintString", n_HelloWorld_PrintString },
	{ "HelloWorld_CheckArgsTest", n_HelloWorld_CheckArgsTest }
};


PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return PLUGIN_SUPPORTS_FLAGS;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (void *(*)(const char *fmt, ...))ppData[PLUGIN_DATA_LOGPRINTF];
	if (NULL == pAMXFunctions || NULL == logprintf)
		return false;
	int plug_ver_major, plug_ver_minor, plug_ver_build;
	pluginutils::SplitVersion(PLUGIN_VERSION, plug_ver_major, plug_ver_minor, plug_ver_build);
	logprintf("  %s plugin v%d.%d.%d is OK", PLUGIN_NAME, plug_ver_major, plug_ver_minor, plug_ver_build);
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	logprintf("  %s plugin was unloaded", PLUGIN_NAME);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx)
{
	if (!pluginutils::CheckIncludeVersion(amx))
		return 0;
	amx_Register(amx, plugin_natives, (int)arraysize(plugin_natives));

	// Native function hooking example.
	bool native_found = pluginutils::ReplaceNative(
		amx, "IsPlayerConnected", hook_IsPlayerConnected, &orig_IsPlayerConnected);
	if (native_found)
		logprintf("IsPlayerConnected hooked successfully");

	return 1;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx)
{
	return AMX_ERR_NONE;
}

/*
PLUGIN_EXPORT int PLUGIN_CALL ProcessTick()
{
	return AMX_ERR_NONE;
}
*/
