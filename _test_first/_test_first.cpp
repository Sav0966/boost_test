// _test_first.cpp: проверка путей заголовков и библиотек BOOST

#include <iostream>
#include <string>

/***
*
* 1) Create system variable BOOSTPATH and set its value to the path of the Boost
* 2) Add $(BOOSTPATH) to VC++ include paths (not additional include path of C/C++)
* 3) Add $(BOOSTPATH)\lib to additional library paths (Linker - Common options)
*
***/

#include <boost/regex.hpp>

int main()
{
	std::string line;
	boost::regex pat("^Subject: (Re: |Aw: )*(.*)");

	while (std::cin)
	{
		std::getline(std::cin, line);
		boost::smatch matches;
		if (boost::regex_match(line, matches, pat))
			std::cout << matches[2] << std::endl;
	}

	return 0;
}
