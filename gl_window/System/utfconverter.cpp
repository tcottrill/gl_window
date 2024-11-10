
#include <Windows.h>
#include "UtfConverter.h"
#include <vector>



 namespace UtfConverter
 {
       //The following Character to/from UTF-8 is from GLFW. win32_init.c -------------

		// Returns a wide string version of the specified UTF-8 string
	WCHAR* FromUTF8(const char* source)
	{
		WCHAR* target;
		int length;

		length = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
		if (!length)
			return NULL;

		target = (WCHAR*) malloc(sizeof(WCHAR) * (length + 1));

		if (!MultiByteToWideChar(CP_UTF8, 0, source, -1, target, length + 1))
		{
			free(target);
			return NULL;
		}

		return target;
	}

	// Returns a UTF-8 string version of the specified wide string
	//
	char* ToUtf8(const WCHAR* source)
	{
		char* target;
		int length;

		length = WideCharToMultiByte(CP_UTF8, 0, source, -1, NULL, 0, NULL, NULL);
		if (!length)
			return NULL;

		target = (char*) malloc(length + 1);

		if (!WideCharToMultiByte(CP_UTF8, 0, source, -1, target, length + 1, NULL, NULL))
		{
			free(target);
			return NULL;
		}

		return target;
	}

	//Taken from CodeSource Example  ---------------------------------------------------------------------------
	 std::wstring FromUtf8(const std::string& utf8string)
	 {
		 int widesize = ::MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, NULL, 0);
		 if (widesize == ERROR_NO_UNICODE_TRANSLATION)
		 {
		 throw std::exception("Invalid UTF-8 sequence.");
		 }
		 if (widesize == 0)
		 {
		 throw std::exception("Error in conversion.");
		 }
  
		 std::vector<wchar_t> resultstring(widesize);
  
		 int convresult = ::MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, &resultstring[0], widesize);
  
		 if (convresult != widesize)
		 {
		 throw std::exception("Error, String not converted to wide!");
		 }
  
	  return std::wstring(&resultstring[0]);
	 }
  

	 std::string ToUtf8(const std::wstring& widestring)
	 {
		 int utf8size = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, NULL, 0, NULL, NULL);
		 if (utf8size == 0)
		 {
		 throw std::exception("Error in conversion.");
		 }
  
		 std::vector<char> resultstring(utf8size);
  
		 int convresult = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, &resultstring[0], utf8size, NULL, NULL);
  
		 if (convresult != utf8size)
		 {
		 throw std::exception("La falla!");
		 }
  
	  return std::string(&resultstring[0]);
	 }

 } 
