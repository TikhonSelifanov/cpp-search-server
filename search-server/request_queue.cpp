#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) : server_(search_server)
{
    count_no_res_ = 0;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status)
{
    return AddFindRequest(raw_query, [status](int id, DocumentStatus stat, int rating){ return stat == status;});
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query)
{
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const
{
    return count_no_res_;
}
