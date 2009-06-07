#include <types.r>
#include <SysTypes.r>
#include "aviffpln.ver"

resource 'vers' (1, purgeable)
{
	TARVER_MAJOR_VERSION, 
	TARVER_MINOR_VERSION,
	beta,
	(TARVER_ULONG32_VERSION && 0xFF),
	verUS,
	TARVER_STRING_VERSION,
	TARVER_STRING_VERSION ", Copyright (C) 1997-98 RealNetworks Corporation."
};