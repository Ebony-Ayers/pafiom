#include "pafiom.hpp"

#include <iostream>
#include <fstream>
#include <memory>

namespace pafiom
{
	FileIOManger::FileIOManger()
	: m_pqueue()
	{
		//
	}
	FileIOManger::~FileIOManger()
	{
		//
	}

	void FileIOManger::submitRequest(FileIORequest* request, FileIOData* data, size_t priority)
	{
		FileIOManger::Job job;
		job.request = request;
		job.data = data;

		this->m_pqueue.enqueue(job, priority);
	}
	void FileIOManger::deleteRequest(FileIORequest* request)
	{
		this->internalDeleteNode(request);
	}
	void FileIOManger::changePriority(FileIORequest* request, size_t newPriotity)
	{
		auto job = this->internalDeleteNode(request);
		if(job.request != nullptr) [[likely]]
		{
			this->m_pqueue.enqueue(job, newPriotity);
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

	inline FileIOManger::Job FileIOManger::internalDeleteNode(FileIORequest* request)
	{
		//to delete the node you need to linearly search though each posible priority and with in that the linked list of itelements
		for(size_t i = 0; i < FileIOManger::numPriorities; i++)
		{
			//iterator over linked list
			bool found = false;
			fsds::FinitePQueue<Job, numPriorities>::Node* node = nullptr;
			fsds::FinitePQueue<Job, numPriorities>::Node* previousNode = nullptr;
			for(node = this->m_pqueue.m_queueHeads[i]; node != nullptr; node = node->next)
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
					this->m_pqueue.m_queueHeads[i] = node->next;
				}
				//last node in linked list
				else if(node->next == nullptr)
				{
					previousNode->next = nullptr;
					this->m_pqueue.m_queueTails[i] = previousNode;
				}
				//neither end of linked list
				else
				{
					previousNode->next = node->next;
					node->next->previous = previousNode;
				}
				this->m_pqueue.m_queueSizes[i]--;

				//copy the node
				FileIOManger::Job job;
				job.request = node->data.request;
				job.data = node->data.data;

				//return node to pqueue's avilable nodes
				this->m_pqueue.m_availableNodes.enqueue(node);

				return job;
			}
		}

		//if the function reaches here then it was unable to find the request
		FileIOManger::Job job;
		job.request = nullptr;
		return job;
	}
}