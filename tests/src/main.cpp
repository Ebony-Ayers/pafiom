#include <iostream>

#include "../../src/pafiom.hpp"

int main(int /*argc*/, const char** /*argv*/)
{
	pafiom::FileIORequest request1;
	request1.fileName = "Request 1";
	request1.openMode = pafiom::FileOpenMode::read;
	request1.isBinary = false;
	request1.start = 0;
	request1.length = 0;
	pafiom::FileIOData data1;

	pafiom::FileIORequest request2;
	request2.fileName = "Request 2";
	request2.openMode = pafiom::FileOpenMode::write;
	request2.isBinary = false;
	request2.start = 0;
	request2.length = 12;
	pafiom::FileIOData data2;

	pafiom::FileIORequest request3;
	request3.fileName = "Request 3";
	request3.openMode = pafiom::FileOpenMode::append;
	request3.isBinary = true;
	request3.start = 24;
	request3.length = 30;
	pafiom::FileIOData data3;

	pafiom::FileIOManger fman;

	fman.submitRequest(&request1, &data1, 1);
	fman.submitRequest(&request2, &data2, 1);
	fman.submitRequest(&request3, &data3, 1);

	fman.changePriority(&request2, 2);
	
	std::cout << fman.m_pqueue.dequeue().request->fileName << std::endl;
	std::cout << fman.m_pqueue.dequeue().request->fileName << std::endl;
	std::cout << fman.m_pqueue.dequeue().request->fileName << std::endl;
	
	return 0;
}
