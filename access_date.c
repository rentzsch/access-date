 /*******************************************************************************
	access_date.c
		Copyright (c) 2007-2009 Jonathan 'Wolf' Rentzsch: <http://rentzsch.com>
		Some rights reserved: <http://creativecommons.org/licenses/by/2.0/>

	***************************************************************************/

#include <Carbon/Carbon.h>
#include <sys/types.h>
#include <sys/stat.h>

#pragma mark Declarations of external entry points

#pragma GCC visibility push(default)

#ifdef __cplusplus
extern "C" {
#endif
    
    OSErr UnixAccessDataEventHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
    OSErr MetadataAccessDataEventHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
    
#ifdef __cplusplus
}
#endif

#pragma GCC visibility pop



#pragma mark Event handlers

OSErr UnixAccessDataEventHandler(const AppleEvent *event, AppleEvent *reply, SRefCon refcon)
{
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
	
	return err;
}

#define CFSafeRelease(VAR) if(VAR) { CFRelease(VAR); VAR = NULL; }

OSErr MetadataAccessDataEventHandler(const AppleEvent *event, AppleEvent *reply, SRefCon refcon)
{	
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
	
	return err;
}