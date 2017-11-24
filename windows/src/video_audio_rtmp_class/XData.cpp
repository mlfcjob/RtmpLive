#include "XData.h"
#include <string.h>

XData::XData()
{
}


XData::~XData()
{
}

void XData::Drop(){
	if (data)
	{
		delete data;
	}
	data = 0;
	size = 0;
}

XData::XData(char *data, int size)
{
	this->data = new char[size];
	memcpy(this->data, data, size);
	this->size = size;
}