// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// Copyright (C) 2012	damysteryman


#ifndef _IOSPATCH_H
#define _IOSPATCH_H

#include <gccore.h>
#include <ogc/machine/processor.h>

#define AHBPROT_DISABLED (read32(0xcd800064) != 0)

int  IOSPATCH_Apply();
void IOSPATCH_SetSignatureChecks(bool);

#endif /* _IOSPATCH_H */
