///
///  Copyright (c) 2008 - 2009 Advanced Micro Devices, Inc.
 
///  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
///  EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
///  WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

/// \file main.c
/// \brief C/C++ ADL sample application
///
/// Demonstrates some basic ADL functions - create, destroy, obtaining adapter and display information.
/// If the display capabilities allow, increases, decreases and restores the brightness of each display

#if defined(HPL_GPU_TEMPERATURE_THRESHOLD) | defined(MAINPROG)
#define LINUX
#include "../../ADL/include/adl_sdk.h"
#include <dlfcn.h>	//dyopen, dlsym, dlclose
#include <stdlib.h>	
#include <string.h>	//memeset
#include <unistd.h>	//sleep

#include <stdio.h>

#include "util_adl.h"

// Definitions of the used function pointers. Add more if you use other ADL APIs
typedef int ( *ADL_MAIN_CONTROL_CREATE )(ADL_MAIN_MALLOC_CALLBACK, int );
typedef int ( *ADL_MAIN_CONTROL_DESTROY )();
typedef int ( *ADL_ADAPTER_NUMBEROFADAPTERS_GET ) ( int* );
typedef int ( *ADL_ADAPTER_ADAPTERINFO_GET ) ( LPAdapterInfo, int );
typedef int ( *ADL_OVERDRIVE5_TEMPERATURE_GET ) ( int, int , ADLTemperature * );
typedef int ( *ADL_ADAPTER_ACTIVE_GET ) ( int, int* );

// Memory allocation function
void* __stdcall ADL_Main_Memory_Alloc ( int iSize )
{
    void* lpBuffer = malloc ( iSize );
    return lpBuffer;
}

ADL_MAIN_CONTROL_CREATE          ADL_Main_Control_Create;
ADL_MAIN_CONTROL_DESTROY         ADL_Main_Control_Destroy;
ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get;
ADL_ADAPTER_ADAPTERINFO_GET      ADL_Adapter_AdapterInfo_Get;
ADL_OVERDRIVE5_TEMPERATURE_GET   ADL_Overdrive5_Temperature_Get;
ADL_ADAPTER_ACTIVE_GET   ADL_Adapter_Active_Get;

int nAdapters;
int* nAdapterIndizes;
void *hDLL;		// Handle to .so library

int adl_temperature_check_init()
{

	
    LPAdapterInfo     lpAdapterInfo = NULL;
    int  iNumberAdapters;

    hDLL = dlopen( "libatiadlxx.so", RTLD_LAZY|RTLD_GLOBAL);

        if (NULL == hDLL)
        {
            printf("ADL library not found!\n");
            return 0;
        }

        ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE) dlsym(hDLL,"ADL_Main_Control_Create");
        ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY) dlsym(hDLL,"ADL_Main_Control_Destroy");
        ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET) dlsym(hDLL,"ADL_Adapter_NumberOfAdapters_Get");
        ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET) dlsym(hDLL,"ADL_Adapter_AdapterInfo_Get");
        ADL_Overdrive5_Temperature_Get = (ADL_OVERDRIVE5_TEMPERATURE_GET) dlsym(hDLL,"ADL_Overdrive5_Temperature_Get");
        ADL_Adapter_Active_Get = (ADL_ADAPTER_ACTIVE_GET) dlsym(hDLL,"ADL_Adapter_Active_Get");
		if ( NULL == ADL_Main_Control_Create || NULL == ADL_Main_Control_Destroy || NULL == ADL_Adapter_NumberOfAdapters_Get || NULL == ADL_Adapter_AdapterInfo_Get || NULL == ADL_Overdrive5_Temperature_Get || NULL == ADL_Adapter_Active_Get)
		{
	       printf("ADL's API is missing!\n");
		   return 0;
		}

        // Initialize ADL. The second parameter is 1, which means:
        // retrieve adapter information only for adapters that are physically present and enabled in the system
        if ( ADL_OK != ADL_Main_Control_Create (ADL_Main_Memory_Alloc, 1) )
		{
	       printf("ADL Initialization Error!\n");
		   return 0;
		}

        // Obtain the number of adapters for the system
        if ( ADL_OK != ADL_Adapter_NumberOfAdapters_Get ( &iNumberAdapters ) )
		{
	       printf("Cannot get the number of adapters!\n");
		   return 0;
		}
		
	if (iNumberAdapters == 0)
	{
		printf("No Adapter found\n");
		return(1);
	}
		
	lpAdapterInfo = (AdapterInfo*) malloc( sizeof(AdapterInfo) * iNumberAdapters);
	if (ADL_Adapter_AdapterInfo_Get(lpAdapterInfo, sizeof(AdapterInfo) * iNumberAdapters) != ADL_OK)
	{
		printf("Error getting adapter info\n");
		return(1);
	}

	for (int j = 0;j < 2;j++)
	{
		nAdapters = 0;
		for ( int i = 0; i < iNumberAdapters; i++ )
		{
			int status;
			if (ADL_Adapter_Active_Get(lpAdapterInfo[i].iAdapterIndex, &status) != ADL_OK)
			{
				printf("Error getting adapter status\n");
				return(1);
			}
			if (status == ADL_TRUE)
			{
				if (j) nAdapterIndizes[nAdapters] = lpAdapterInfo[i].iAdapterIndex;
				nAdapters++;
			}
		}
		if (j == 0) nAdapterIndizes = new int[nAdapters];
	}
	free(lpAdapterInfo);
	return(0);
}

int adl_temperature_check_run(double* max_temperature, int verbose)
{
	*max_temperature = 0.;
	if (verbose) printf("Temperatures:");
	for (int i = 0;i < nAdapters;i++)
	{
		ADLTemperature temp;
		temp.iSize = sizeof(temp);
		if (ADL_Overdrive5_Temperature_Get(nAdapterIndizes[i], 0, &temp) != ADL_OK)
		{
			printf("Error reading temperature from adapter %d\n", i);
			return(1);
		}
		const double temperature = temp.iTemperature / 1000.;
		if (verbose) printf(" %f", temperature);
		if (temperature > *max_temperature) *max_temperature = temperature;
        }
        if (verbose) printf("\n");
        return(0);
}

int adl_temperature_check_exit()
{
    ADL_Main_Control_Destroy ();
    dlclose(hDLL);

    return(0);
}

#ifdef MAINPROG
int main (int argc, char** argv)
{
	double temperature;
	if (adl_temperature_check_init())
	{
		printf("Error initializing ADL\n");
		return(1);
	}
	if (adl_temperature_check_run(&temperature, 1))
	{
		printf("Error running ADL temperature check\n");
		return(1);
	}
	printf("Maximum Temperature: %f\n", temperature);
	if (adl_temperature_check_exit())
	{
		printf("Error exiting ADL\n");
		return(1);
	}
}
#endif

#endif
