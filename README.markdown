#access date

A simple AppleScript Addition (osax) for Mac OS X Leopard 10.5 and Snow Leopard 10.6 that grants AppleScript scripts fast, native access to the two forms of a file's last access date:

* **unix access date:** the "time of last access" field (`st_atimespec`) from calling `stat(2)`.

* **metadata access date:** the `kMDItemLastUsedDate` attribute from Metadata.framework.

The *unix access date* tends to be more accurate than the *metadata access date* -- ***too*** accurate. Even Quick-Looking a file will update its *unix access date*, which makes sense but often is too twitchy for our purposes.

So my scripts pretty much exclusively use *metadata access date*, which is far less twitchy.