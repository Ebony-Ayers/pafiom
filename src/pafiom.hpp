#pragma once
#include "pafiom_options.hpp"

#include <iostream>
#include "../lib/fast-thread-syncronization/src/fts.hpp"

namespace pafiom
{
	enum struct FileOpenMode
	{
		read = std::ios::in,
		write = std::ios::out,
		append = std::ios::app
	};
	
	struct FileIORequest
	{
		const char* fileName;
		FileOpenMode openMode;
		bool isBinary;
		size_t start = 0;
		size_t length = 0;							//if length is 0 the entire file will be read
	};

	struct FileIOData
	{
		char* data = nullptr;						//if data is nullptr a new buffer will be allocate to store it
		fts::Signal isAvailable = fts::Signal();
	};

	class FileIOManger
	{
		public:
			const size_t numPriorities = PAFIOM_NUM_PRIORITIES;

			FileIOManger();
			FileIOManger(const FileIOManger&) = delete;
			FileIOManger(FileIOManger&&) = delete;
			~FileIOManger();

			int submitRequest(FileIORequest* request, FileIOData* data, size_t priority);
			int deleteRequest(FileIORequest* request);
			int changePriority(FileIORequest*, size_t newPriotity);

			size_t getFileLength(const char* fileName);
		
		private:
			struct Job
			{
				FileIORequest* request;
				FileIOData* data;
			};
	};
}