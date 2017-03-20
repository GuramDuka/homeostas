#ifndef VERSION_H_INCLUDED
#define VERSION_H_INCLUDED

namespace spacenet{
	
	//Date Version Types
	static const char VERSION_DATE[] = "03";
	static const char VERSION_MONTH[] = "03";
	static const char VERSION_YEAR[] = "2017";
	static const char VERSION_UBUNTU_VERSION_STYLE[] =  "17.03";
	
	//Software Status
	static const char VERSION_STATUS[] =  "Alpha";
	static const char VERSION_STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long VERSION_MAJOR  = 1;
	static const long VERSION_MINOR  = 0;
	static const long VERSION_BUILD  = 2;
	static const long VERSION_REVISION  = 13;
	
	//Miscellaneous Version Types
	static const long VERSION_BUILDS_COUNT  = 14;
	#define VERSION_RC_FILEVERSION 1,0,2,13
	#define VERSION_RC_FILEVERSION_STRING "1, 0, 2, 13\0"
	static const char VERSION_FULLVERSION_STRING [] = "1.0.2.13";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long VERSION_BUILD_HISTORY  = 2;
	

}
#endif //VERSION_H_INCLUDED
