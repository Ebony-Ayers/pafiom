#include "pafiom.hpp"

#include <iostream>
#include <fstream>
#include <memory>

namespace pafiom
{
	FileIOManger::FileIOManger()
	: m_fileJobQueue(), m_decodeJobQueue(), m_numActiveThreads(1), m_numMaxThreads(FileIOManger::sm_maxWorkerThreads), m_controlLock(), m_threads(), m_sleepSignal(), m_workerThreadShouldQuit(false)
	{
		//this->m_threads[0] = std::thread(FileIOManger::workerThreadFunction, this);
		//this->m_threads[0].detach();
	}
	FileIOManger::~FileIOManger()
	{
		//
	}

	void FileIOManger::submitRequest(FileIORequest* request, FileIOData* data, const size_t& priority)
	{
		FileIOManger::Job job;
		job.request = request;
		job.data = data;

		FTS_READ_WRITE_WRITE_LOCKGUARD(this->m_controlLock);
		this->m_fileJobQueue.enqueue(job, priority);
	}
	void FileIOManger::deleteRequest(FileIORequest* request)
	{
		FTS_READ_WRITE_WRITE_LOCKGUARD(this->m_controlLock);
		this->internalDeleteNode(request);
	}
	void FileIOManger::changePriority(FileIORequest* request, const size_t& newPriotity)
	{
		FTS_READ_WRITE_WRITE_LOCKGUARD(this->m_controlLock);
		auto job = this->internalDeleteNode(request);
		if(job.request != nullptr) [[likely]]
		{
			this->m_fileJobQueue.enqueue(job, newPriotity);
		}
	}

	size_t FileIOManger::getFileLength(const char* fileName)
	{
		size_t result;
		std::ifstream fileHandle;
		fileHandle.open(fileName, std::ios::ate | std::ios::binary);
		result = static_cast<size_t>(fileHandle.tellg());
		fileHandle.close();
		return result;
	}

	void FileIOManger::setThreadLimit(const size_t& newMax)
	{
		FTS_READ_WRITE_WRITE_LOCKGUARD(this->m_controlLock);
		this->m_numMaxThreads = newMax;
	}

	inline FileIOManger::Job FileIOManger::internalDeleteNode(FileIORequest* request)
	{
		//to delete the node you need to linearly search though each posible priority and with in that the linked list of itelements
		for(size_t i = 0; i < FileIOManger::numPriorities; i++)
		{
			//iterator over linked list
			bool found = false;
			fsds::FinitePQueue<Job, numPriorities>::Node* node = nullptr;
			fsds::FinitePQueue<Job, numPriorities>::Node* previousNode = nullptr;
			for(node = this->m_fileJobQueue.m_queueHeads[i]; node != nullptr; node = node->next)
			{
				if(node->data.request == request)
				{
					found = true;
					break;
				}
				previousNode = node;
			}

			if(found)
			{
				//first element in linked list
				if(previousNode == nullptr)
				{
					node->previous = nullptr;
					this->m_fileJobQueue.m_queueHeads[i] = node->next;
				}
				//last node in linked list
				else if(node->next == nullptr)
				{
					previousNode->next = nullptr;
					this->m_fileJobQueue.m_queueTails[i] = previousNode;
				}
				//neither end of linked list
				else
				{
					previousNode->next = node->next;
					node->next->previous = previousNode;
				}
				this->m_fileJobQueue.m_queueSizes[i]--;

				//copy the node
				FileIOManger::Job job;
				job.request = node->data.request;
				job.data = node->data.data;

				//return node to pqueue's avilable nodes
				this->m_fileJobQueue.m_availableNodes.enqueue(node);

				return job;
			}
		}

		//repeat the previous code for the decode jobs queue
		for(size_t i = 0; i < FileIOManger::numPriorities; i++)
		{
			//iterator over linked list
			bool found = false;
			fsds::FinitePQueue<Job, numPriorities>::Node* node = nullptr;
			fsds::FinitePQueue<Job, numPriorities>::Node* previousNode = nullptr;
			for(node = this->m_decodeJobQueue.m_queueHeads[i]; node != nullptr; node = node->next)
			{
				if(node->data.request == request)
				{
					found = true;
					break;
				}
				previousNode = node;
			}

			if(found)
			{
				//first element in linked list
				if(previousNode == nullptr)
				{
					node->previous = nullptr;
					this->m_decodeJobQueue.m_queueHeads[i] = node->next;
				}
				//last node in linked list
				else if(node->next == nullptr)
				{
					previousNode->next = nullptr;
					this->m_decodeJobQueue.m_queueTails[i] = previousNode;
				}
				//neither end of linked list
				else
				{
					previousNode->next = node->next;
					node->next->previous = previousNode;
				}
				this->m_decodeJobQueue.m_queueSizes[i]--;

				//copy the node
				FileIOManger::Job job;
				job.request = node->data.request;
				job.data = node->data.data;

				//return node to pqueue's avilable nodes
				this->m_decodeJobQueue.m_availableNodes.enqueue(node);

				return job;
			}
		}

		//if the function reaches here then it was unable to find the request
		FileIOManger::Job job;
		job.request = nullptr;
		return job;
	}

	void FileIOManger::workerThreadFunction(FileIOManger* pThis)
	{
		while(true)
		{
			FileIOManger::Job job;
			size_t jobPriority;
			//op = 0: file job
			//op = 1: decode job
			//op = 2: sleep
			char op;
			{
				//detect if the thread has been ordered to stop
				FTS_READ_WRITE_WRITE_LOCKGUARD(pThis->m_controlLock);
				if(pThis->m_workerThreadShouldQuit) [[unlikely]]
				{
					return;
				}

				//get the job, op and priority
				if((pThis->m_fileJobQueue.getCurrentHighestPriority() < pThis->m_decodeJobQueue.getCurrentHighestPriority()) && (!pThis->m_fileJobQueue.isEmpty()))
				{
					jobPriority = pThis->m_fileJobQueue.getCurrentHighestPriority();
					job = pThis->m_fileJobQueue.dequeue();
					op = 0;
				}
				else if(!pThis->m_decodeJobQueue.isEmpty())
				{
					jobPriority = pThis->m_decodeJobQueue.getCurrentHighestPriority();
					job = pThis->m_decodeJobQueue.dequeue();
					op = 1;
				}
				else
				{
					//the code for sleeping is outside this scope so that the sleep happens after the lock is released so that other threads can aquire the lock
					op = 2;
				}
			}
			if(op == 2) [[unlikely]]
			{
				pThis->m_sleepSignal.wait();
				continue;
			}

			//file job
			if(op == 0)
			{
				std::fstream file(job.request->fileName, static_cast<std::ios_base::openmode>(job.request->openMode));
				if((static_cast<std::ios_base::openmode>(job.request->openMode) & std::ios::in) != 0)
				{
					file.seekg(static_cast<std::streamoff>(job.request->start));
					if(job.data->data == nullptr)
					{
						job.data->data = reinterpret_cast<char*>(std::aligned_alloc(64, sizeof(char) * job.request->length));
					}
					file.read(job.data->data, static_cast<std::streamsize>(job.request->length));
				}
				else
				{
					file.seekp(static_cast<std::streamoff>(job.request->start));
					file.write(job.data->data, static_cast<std::streamsize>(job.request->length));
				}
				file.close();
				
				//if the job needs decoding put it in the decode queue otherwise mark it as done
				if(job.request->fileDecodeFunction != nullptr)
				{
					FTS_READ_WRITE_WRITE_LOCKGUARD(pThis->m_controlLock);

					pThis->m_decodeJobQueue.enqueue(job, jobPriority);
				}
				else
				{
					if(job.data->operationCompleteCallback != nullptr) [[unlikely]]
					{
						job.data->operationCompleteCallback(job.data);
					}
				}
			}
			//decode job
			else
			{
				char* output = nullptr;
				void* outputPrameters = nullptr;
				job.request->fileDecodeFunction(job.data->data, job.request->fileDecodeParameters, output, outputPrameters);
				
				job.data->data = output;
				job.data->fileProperties = outputPrameters;
				if(job.data->operationCompleteCallback != nullptr) [[unlikely]]
				{
					job.data->operationCompleteCallback(job.data);
				}
			}
		} // while(true)
	}

	void FileIOManger::workerDecodeFunction(FileIOManger::Job& job)
	{
		char* output = nullptr;
		void* outputPrameters = nullptr;
		job.request->fileDecodeFunction(job.data->data, job.request->fileDecodeParameters, output, outputPrameters);
		
		job.data->data = output;
		job.data->fileProperties = outputPrameters;
		if(job.data->operationCompleteCallback != nullptr) [[unlikely]]
		{
			job.data->operationCompleteCallback(job.data);
		}
	}
} // namespace pafiom