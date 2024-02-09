#pragma once
#include "animation_object.h"
#include <string>

template<typename T>
void AppendData(const T& data, std::vector<char>& buffer)
{
	const char* p_value = (const char*)&data;

	for (size_t i = 0; i < sizeof(T); i++)
		buffer.push_back(*(p_value + i));
}

template<typename T>
void ReadData(const std::vector<char>& buffer, size_t& inoutBufferIndex, T& data)
{
	char* p_data = (char*)&data;

	for (size_t i = 0; i < sizeof(T); i++)
		*(p_data + i) = buffer[inoutBufferIndex++];
}

void AppendAnimationObjectToBuffer(const AnimationObject& object, std::vector<char>& buffer);
void ReadAnimationObjectFromBuffer(const std::vector<char>& buffer, size_t& inoutBufferIndex, AnimationObject& outObject);