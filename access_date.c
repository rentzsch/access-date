 /*******************************************************************************
	access_date.c
		Copyright (c) 2007 Jonathan 'Wolf' Rentzsch: <http://rentzsch.com>
		Some rights reserved: <http://creativecommons.org/licenses/by/2.0/>

	***************************************************************************/

#include <Carbon/Carbon.h>
#include "TermCodes.h"
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __cplusplus
	#define EXTERN_C extern "C"
#else
	#define EXTERN_C
#endif

UInt32			gAdditionReferenceCount = 0;
CFBundleRef		gAdditionBundle;

// =============================================================================
// == Entry points.

static OSErr InstallMyEventHandlers();
static void RemoveMyEventHandlers();

EXTERN_C OSErr SAInitialize(CFBundleRef theBundle)
{
	/*  Typically, scripting additions either totally succeed to load or totally fail.  This is usually, but not always, the right thing to do -- if you had an addition where part of it relied on a shared library, but part didn't, you might want to run in a reduced mode if the library could not be found. */

	gAdditionBundle = theBundle;  // no retain needed.

	// Any other setup you need here...

	return InstallMyEventHandlers();
}

EXTERN_C void SATerminate()
{
	// Release anything you allocated in SAInitialize here...

	RemoveMyEventHandlers();
}

EXTERN_C Boolean SAIsBusy()
{
	return gAdditionReferenceCount != 0;
}

// =============================================================================

OSErr UnixAccessDataEventHandler(const AppleEvent *event, AppleEvent *reply, long refcon)
{
	++gAdditionReferenceCount;
	
	OSErr err = noErr;
	
	FSRef fsref;
	if (!err) {
		DescType actualType;
		Size actualSize;
		err = AEGetParamPtr(event, keyDirectObject, typeFSRef, &actualType, &fsref, sizeof(fsref), &actualSize);
	}
	char fspath[PATH_MAX];
	if (!err) {
		err = FSRefMakePath(&fsref, (UInt8*)fspath, sizeof(fspath));
	}
	if (!err) {
		struct stat sb;
		if (stat(fspath, &sb)) {
			err = paramErr;
		} else {
			LongDateTime time = sb.st_atimespec.tv_sec;
			//fprintf(stderr, "^^sb.st_atimespec.tv_sec: %ld\n", sb.st_atimespec.tv_sec);
			//fprintf(stderr, "^^(unix) time: %lld\n", time);
			
			time += 2082844800; // the number of seconds between the unix epoch (1/1/1970) and the classic mac epoch (1/1/1904).
								//fprintf(stderr, "^^(classic mac) time: %lld\n", time);
			
			CFTimeZoneRef tz = CFTimeZoneCopyDefault();
			time += CFTimeZoneGetSecondsFromGMT(tz, CFAbsoluteTimeGetCurrent());
			CFRelease(tz);
			//fprintf(stderr, "^^(classic mac + gmt) time: %lld\n", time);
			
			err = AEPutParamPtr(reply,
								keyAEResult,
								cLongDateTime,
								&time,
								sizeof(time));
		}
	}
	
	--gAdditionReferenceCount;
	return err;
}

#define CFSafeRelease(VAR) if(VAR) { CFRelease(VAR); VAR = NULL; }

OSErr MetadataAccessDataEventHandler(const AppleEvent *event, AppleEvent *reply, long refcon)
{
	++gAdditionReferenceCount;
	
	OSErr err = noErr;
	
	//	Pull out the AppleEvent's direct-object-param FSRef.
	FSRef fsref;
	if (!err) {
		DescType actualType;
		Size actualSize;
		err = AEGetParamPtr(event, keyDirectObject, typeFSRef, &actualType, &fsref, sizeof(fsref), &actualSize);
	}
	//	FSRef -> char[] posix path.
	char fspath[PATH_MAX];
	if (!err) {
		err = FSRefMakePath(&fsref, (UInt8*)fspath, sizeof(fspath));
	}
	//	char[] posix path -> CFString path.
	CFStringRef fspathStr = NULL;
	if (!err) {
		fspathStr = CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, fspath);
		if (!fspathStr) {
			//fprintf(stderr, "^^CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, \"%s\") failed\n", fspath);
			err = coreFoundationUnknownErr;
		}
	}
	//	CFString path -> MDItem.
	MDItemRef mdItem = NULL;
	if (!err) {
		mdItem = MDItemCreate(kCFAllocatorDefault, fspathStr);
		if (!mdItem) {
			//fprintf(stderr, "^^MDItemCreate(kCFAllocatorDefault, \"%s\") failed\n", fspath);
			err = coreFoundationUnknownErr;
		}
	}
	/*
	CFArrayRef attributeNames = NULL;
	if (!err) {
		CFArrayRef attributeNames = MDItemCopyAttributeNames(mdItem);
		if (!attributeNames)
			err = coreFoundationUnknownErr;
	}*/
	CFDateRef lastUsedDate = NULL;
	if (!err) {
		lastUsedDate = MDItemCopyAttribute(mdItem, kMDItemLastUsedDate);
		if (!lastUsedDate) {
			//fprintf(stderr, "^^MDItemCopyAttribute(mdItem, kMDItemLastUsedDate) failed\n");
			err = coreFoundationUnknownErr;
		}
	}
	LongDateTime time;
	if (!err) {
		err = UCConvertCFAbsoluteTimeToLongDateTime(CFDateGetAbsoluteTime(lastUsedDate), &time);
	}
	if (!err) {
		err = AEPutParamPtr(reply,
							keyAEResult,
							cLongDateTime,
							&time,
							sizeof(time));
	}
	
	//	Clean up.
	CFSafeRelease(lastUsedDate);
	//CFSafeRelease(attributeNames);
	CFSafeRelease(mdItem);
	CFSafeRelease(fspathStr);
	
	--gAdditionReferenceCount;
	return err;
}

// -----------------------------------------------------------------------------
// -- Event handler data.

struct AEEventHandlerInfo {
	FourCharCode			evClass, evID;
	AEEventHandlerProcPtr	proc;
};
typedef struct AEEventHandlerInfo AEEventHandlerInfo;

static const AEEventHandlerInfo gEventInfo[] = {
	{ kRedShedSuite, kRedShedSuiteUnixAccessDateCommand,		UnixAccessDataEventHandler },
	{ kRedShedSuite, kRedShedSuiteMetadataAccessDateCommand,	MetadataAccessDataEventHandler }
	// Add more suite/event/handler triplets here if you define more than one command.
};
#define kEventHandlerCount  (sizeof(gEventInfo) / sizeof(AEEventHandlerInfo))

static AEEventHandlerUPP gEventUPPs[kEventHandlerCount];

// =============================================================================

static OSErr InstallMyEventHandlers()
{
	OSErr		err;
	size_t		i;
	
	for (i = 0; i < kEventHandlerCount; ++i) {
		if ((gEventUPPs[i] = NewAEEventHandlerUPP(gEventInfo[i].proc)) != NULL)
			err = AEInstallEventHandler(gEventInfo[i].evClass, gEventInfo[i].evID, gEventUPPs[i], 0, true);
		else
			err = memFullErr;
		
		if (err != noErr) {
			SATerminate();  // Call the termination function ourselves, because the loader won't once we fail.
			return err;
		}
	}

	return noErr; 
}

// -----------------------------------------------------------------------------

static void RemoveMyEventHandlers()
{
	OSErr		err;
	size_t		i;

	for (i = 0; i < kEventHandlerCount; ++i) {
		if (gEventUPPs[i] &&
			(err = AERemoveEventHandler(gEventInfo[i].evClass, gEventInfo[i].evID, gEventUPPs[i], true)) == noErr)
		{
			DisposeAEEventHandlerUPP(gEventUPPs[i]);
			gEventUPPs[i] = NULL;
		}
	}
}
