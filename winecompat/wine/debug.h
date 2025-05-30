#pragma once

#define WINE_DEFAULT_DEBUG_CHANNEL(x) static int g_dbg_channel_ ## x;
#define WARN(fmt, ...) ((void)0)
#define TRACE(fmt, ...) ((void)0)
#define FIXME(fmt, ...) ((void)0)
#define ERR(fmt, ...) ((void)0)

#define _O_ACCMODE (_O_RDONLY | _O_WRONLY | _O_RDWR)
