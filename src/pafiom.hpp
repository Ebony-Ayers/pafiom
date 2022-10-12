#pragma once
#include "pafiom_options.hpp"

#include <iostream>
#include <array>
#include "../lib/fast-thread-syncronization/src/fts.hpp"
#include "../lib/fsds/src/fsds.hpp"

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
			static const size_t numPriorities = PAFIOM_NUM_PRIORITIES;

			FileIOManger();
			FileIOManger(const FileIOManger&) = delete;
			FileIOManger(FileIOManger&&) = delete;
			~FileIOManger();

			void submitRequest(FileIORequest* request, FileIOData* data, size_t priority);
			void deleteRequest(FileIORequest* request);
			void changePriority(FileIORequest* request, size_t newPriotity);

			size_t getFileLength(const char* fileName);
		
		//private:
			static const size_t sm_defaulAllocataion = PAFIOM_NUM_PRIORITIES * 2;
			static const size_t sm_defaultReadSize = PAFIOM_READ_SIZE;
			static const size_t sm_numWorkerThreads = PAFIOM_NUM_THREADS;

			struct Job
			{
				FileIORequest* request;
				FileIOData* data;
			};
			struct TupleNodePair
			{
				fsds::FinitePQueue<Job, numPriorities>::Node* node;
				fsds::FinitePQueue<Job, numPriorities>::Node* previousNode;
				size_t priority;
			};
			
			inline Job internalDeleteNode(FileIORequest* request);
			
			fsds::FinitePQueue<Job, numPriorities> m_pqueue;

			friend class fsds::FinitePQueue<Job, numPriorities>;
	};
}