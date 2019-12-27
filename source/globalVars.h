#ifndef __globalVars_h__
#define __globalVars_h__

#include "mbed.h"
#include <stdio.h>
#include "SDIOBlockDevice.h"
#include "FATFileSystem.h"


#ifdef COMPONENT_SPIF
#include "SPIFBlockDevice.h"
#include "LittleFileSystem.h"

extern SPIFBlockDevice spif;
extern LittleFileSystem lfs;

void formatSPIFlash(); 
void print_SPIF_info();
#endif

void print_log(const char *format, ... );

typedef struct 
{
    float adcValues[4];
} GlobalVars;

extern SDIOBlockDevice bd;
extern FATFileSystem fs;

extern GlobalVars globalVars;

void print_dir(FileSystem *fs, const char* dirname);

#endif