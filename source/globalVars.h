#ifndef __globalVars_h__
#define __globalVars_h__

#include "mbed.h"
#include "SDIOBlockDevice.h"
#include "SPIFBlockDevice.h"
#include "FATFileSystem.h"
#include "LittleFileSystem.h"


typedef struct 
{
    float adcValues[4];
} GlobalVars;

extern SDIOBlockDevice bd;
extern FATFileSystem fs;

extern SPIFBlockDevice spif;
extern LittleFileSystem lfs;

extern GlobalVars globalVars;

void formatSPIFlash();
void print_dir(FileSystem *fs, const char* dirname);
void print_SPIF_info();

#endif