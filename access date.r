#include <Carbon/Carbon.r>

#define Reserved8   reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved
#define Reserved12  Reserved8, reserved, reserved, reserved, reserved
#define Reserved13  Reserved12, reserved
#define dp_none__   noParams, "", directParamOptional, singleItem, notEnumerated, Reserved13
#define reply_none__   noReply, "", replyOptional, singleItem, notEnumerated, Reserved13
#define synonym_verb__ reply_none__, dp_none__, { }
#define plural__    "", {"", kAESpecialClassProperties, cType, "", reserved, singleItem, notEnumerated, readOnly, Reserved8, noApostrophe, notFeminine, notMasculine, plural}, {}

resource 'aete' (0, "Dictionary") {
	0x1,  // major version
	0x0,  // minor version
	english,
	roman,
	{
		"Red Shed Software Suite",
		"A suite of AppleScript Additions from Red Shed Software",
		'RShd',
		1,
		1,
		{
			/* Events */

			"unix access date",
			"returns a file or folder's last unix (stat(2)) access date",
			'RShd', 'UAcc',
			'ldt ',
			"the specified file or folder's last access date",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'file',
			"an alias or file reference to the file or folder",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"metadata access date",
			"returns a file or folder's last metadata (kMDItemLastUsedDate) access date",
			'RShd', 'MAcc',
			'ldt ',
			"the specified file or folder's last access date",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'file',
			"an alias or file reference to the file or folder",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			}
		},
		{
			/* Classes */

		},
		{
			/* Comparisons */
		},
		{
			/* Enumerations */
		}
	}
};
