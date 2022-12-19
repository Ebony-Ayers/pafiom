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
		binaryRead = std::ios::in | std::ios::binary,
		write = std::ios::out,
		binaryWrite = std::ios::out | std::ios::binary,
		append = std::ios::app,
		binaryAppend = std::ios::app | std::ios::binary
	};

	enum struct FileType
	{
		binary,
		text,
		image,
		mesh,
		audio
	};

	struct FileIOData;
	typedef void (*FileDecodeFunction)(const char* const rawInput, const void* const parameters, char* decodedOutput, void* outputPrameters);
	typedef void (*OperationCompleteCallback)(const FileIOData* const fileData);
	
	struct FileIORequest
	{
		const char* fileName;
		FileOpenMode openMode;
		size_t start;
		size_t length;
		FileDecodeFunction fileDecodeFunction = nullptr;	//optional
		void* fileDecodeParameters = nullptr;				//optional
	};

	struct FileIOData
	{
		char* data = nullptr;								//if data is nullptr a new buffer will be allocated with std::aligned_alloc andmust be freed with std::free() by the user
		void* fileProperties = nullptr;						//optional
		OperationCompleteCallback operationCompleteCallback = nullptr;	//NOT OPTIONAL
		void* operationCompleteCallbackParameters;
	};

	class FileIOManger
	{
		public:
			static const size_t numPriorities = PAFIOM_NUM_PRIORITIES;

			FileIOManger();
			FileIOManger(const FileIOManger&) = delete;
			FileIOManger(FileIOManger&&) = delete;
			~FileIOManger();

			void submitRequest(FileIORequest* request, FileIOData* data, const size_t& priority);
			void deleteRequest(FileIORequest* request);
			void changePriority(FileIORequest* request, const size_t& newPriotity);

			size_t getFileLength(const char* fileName);

			void setThreadLimit(const size_t& newMax);
		
		//private:
			static const size_t sm_defaulAllocataion = PAFIOM_NUM_PRIORITIES * 2;
			static const size_t sm_maxWorkerThreads  = PAFIOM_MAX_THREADS;

			struct Job
			{
				FileIORequest* request;
				FileIOData* data;
			};
			struct TupleNodePair
			{
				fsds::FinitePQueue<Job, FileIOManger::numPriorities>::Node* node;
				fsds::FinitePQueue<Job, FileIOManger::numPriorities>::Node* previousNode;
				size_t priority;
			};
			
			inline Job internalDeleteNode(FileIORequest* request);

			static void workerThreadFunction(FileIOManger* pThis);
			static void workerDecodeFunction(Job& job);
			
			//queue of jobs
			fsds::FinitePQueue<Job, FileIOManger::numPriorities> m_fileJobQueue;
			fsds::FinitePQueue<Job, FileIOManger::numPriorities> m_decodeJobQueue;
			size_t m_numActiveThreads;
			size_t m_numMaxThreads;
			fts::ReadWriteLock m_controlLock;
			std::array<std::thread, FileIOManger::sm_maxWorkerThreads> m_threads;
			//if there is nothing to read then the signal will sleep the thread until it is awoken
			fts::Signal m_sleepSignal;
			bool m_workerThreadShouldQuit;

			friend class fsds::FinitePQueue<Job, numPriorities>;
	}; // class FileIOData
} // namespace pafiom