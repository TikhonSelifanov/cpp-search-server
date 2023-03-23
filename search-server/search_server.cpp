#include "search_server.h"

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const
{
    std::vector<std::string> plus_words;
    Query query_words = ParseQuery(raw_query);

    for (const std::string& minus_word : query_words.minus_words)
    {
        if (word_to_document_id_freqs_.find(minus_word) != word_to_document_id_freqs_.end())
        {
            if (word_to_document_id_freqs_.at(minus_word).find(document_id) != word_to_document_id_freqs_.at(minus_word).end())
            {
                return {std::vector<std::string> {}, id_doc_info_.at(document_id).status};
            }
        }
    }

    for (const std::string& plus_word : query_words.plus_words)
    {
        if (word_to_document_id_freqs_.find(plus_word) != word_to_document_id_freqs_.end())
        {
            if (word_to_document_id_freqs_.at(plus_word).find(document_id) != word_to_document_id_freqs_.at(plus_word).end())
            {
                plus_words.push_back(plus_word);
            }
        }
    }
    return {plus_words, id_doc_info_.at(document_id).status};
}

void SearchServer::AddDocument(const int document_id, const std::string& document, const DocumentStatus& stat, const std::vector<int>& ratings)
{
    if (document_id < 0)
    {
        throw std::invalid_argument("Id is less than 0");
    }

    if (id_doc_info_.find(document_id) != id_doc_info_.end())
    {
        throw std::invalid_argument("Document with such an id already exists");
    }

    docs_id_.push_back(document_id);

    std::vector<std::string> words = SplitIntoWordsNoStop(document);

    int averageRating = ComputeAverageRating(ratings);
    id_doc_info_[document_id].rating = averageRating;

    id_doc_info_[document_id].status = stat;

    for (const std::string& word : words)
    {
        // аккумулируем TF для всех слов
        word_to_document_id_freqs_[word][document_id] += 1.0 / words.size();
    }
    ++document_count_;
}
