#pragma once

class ResourceData
{
public:
	const unsigned char * data;
	const unsigned int size;

	ResourceData(const unsigned char * data, const unsigned int size):
		data(data),
		size(size)
	{ }

	ResourceData():
		data(nullptr),
		size(0)
	{ }

	bool HasData() const
	{
		return data != nullptr;
	}
};
