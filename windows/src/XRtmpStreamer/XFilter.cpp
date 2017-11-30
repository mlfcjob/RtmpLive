#include "XFilter.h"
#include "XBilateralFilter.h"
#include <iostream>

using namespace std;

bool XFilter::Set(std::string key, double value)
{
	if (paras.find(key) == paras.end())
	{
		cout << "para " << key << "is not supported! " << endl;
		return false;
	}

	paras[key] = value;
	return true;
}

XFilter::XFilter()
{
}


XFilter::~XFilter()
{
}

XFilter *XFilter::Get(XFilterType type)
{
	static XBilateralFilter  xbf;
	switch (type)
	{
	case XBILATERAL: //Ë«±ßÂË²¨
		return &xbf;
		break;
	default:
		break;
	}

	return 0;
}