#pragma once
#include <queue>
#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) : server_(search_server)
    {
        count_no_res_ = 0;
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate)
    {
        std::vector<Document> vec = server_.FindTopDocuments(raw_query, document_predicate);
        if (vec.empty())
        {
            ++count_no_res_;
        }
        requests_.push_front({vec.empty()});

        if (requests_.size() > min_in_day_)
        {
            if (requests_.back().is_empty)
            {
                --count_no_res_;
            }
            requests_.pop_back();
        }

        return vec;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status)
    {
        return AddFindRequest(raw_query, [status](int id, DocumentStatus stat, int rating){ return stat == status;});
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query)
    {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }
    int GetNoResultRequests() const
    {
        return count_no_res_;
    }
private:
    struct QueryResult
    {
        bool is_empty;
    };
    const SearchServer& server_;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int count_no_res_;
};
