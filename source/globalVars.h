#ifndef __globalVars_h__
#define __globalVars_h__

#include "mbed.h"

/*
#include "SDIOBlockDevice.h"
#include "FATFileSystem.h"

#ifdef COMPONENT_SPIF
#include "SPIFBlockDevice.h"
#include "LittleFileSystem.h"

extern SPIFBlockDevice spif;
extern LittleFileSystem lfs;
extern SDIOBlockDevice bd;
extern FATFileSystem fs;

void formatSPIFlash(FileSystem *fs); 
void print_SPIF_info();

void print_dir(FileSystem *fs, const char* dirname);
#endif
*/

typedef struct 
{
    float adcValues[4];
} GlobalVars;

extern GlobalVars globalVars;


#endif