#include <iostream>

#include "../../src/pafiom.hpp"

void debugDecodeFunction(const char* const rawInput, const void* const parameters, char* decodedOutput, void* outputPrameters)
{
	return;
}
void debugCallbackFunction(const pafiom::FileIOData* const fileData)
{
	return;
}

int main(int /*argc*/, const char** /*argv*/)
{
	pafiom::FileIORequest request1;
	request1.fileName = "Request 1";
	request1.openMode = pafiom::FileOpenMode::read;
	request1.start = 0;
	request1.length = 0;
	pafiom::FileIOData data1;
	data1.operationCompleteCallback = debugCallbackFunction;
	data1.operationCompleteCallbackParameters = nullptr;

	pafiom::FileIORequest request2;
	request2.fileName = "Request 2";
	request2.openMode = pafiom::FileOpenMode::write;
	request2.start = 0;
	request2.length = 12;
	pafiom::FileIOData data2;
	data2.operationCompleteCallback = debugCallbackFunction;
	data2.operationCompleteCallbackParameters = nullptr;

	pafiom::FileIORequest request3;
	request3.fileName = "Request 3";
	request3.openMode = pafiom::FileOpenMode::append;
	request3.start = 24;
	request3.length = 30;
	request3.fileDecodeFunction = debugDecodeFunction;
	pafiom::FileIOData data3;
	data3.operationCompleteCallback = debugCallbackFunction;
	data3.operationCompleteCallbackParameters = nullptr;

	pafiom::FileIOManger fman;
	
	fman.submitRequest(&request1, &data1, 1);
	fman.submitRequest(&request2, &data2, 1);
	fman.submitRequest(&request3, &data3, 1);
	
	fman.changePriority(&request2, 2);
	
	pafiom::FileIOManger::workerThreadFunction(&fman);
	pafiom::FileIOManger::workerThreadFunction(&fman);
	
	return 0;
}
