#pragma once
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <map>
#include <numeric>
#include <stdexcept>
#include <cmath>
#include "string_processing.h"
#include "document.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

class SearchServer
{
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        if (any_of(stop_words_.begin(), stop_words_.end(), [](const std::string& word) {return !IsValidWord(word);}))
        {
            throw std::invalid_argument("Word contains symbols with codes from 0 to 31");
        }
    }

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {

    }

    int GetDocumentCount() const
    {
        return document_count_;
    }

    int GetDocumentId(int index) const
    {
        if (index < 0 || index > document_count_)
        {
            throw std::out_of_range("Id is out of range");
        }
        return docs_id_[index];
    }

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    void AddDocument(const int document_id, const std::string& document, const DocumentStatus& stat, const std::vector<int>& ratings);

    template <typename T>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, T predicate) const // задано условие
    {
        const Query query_words = ParseQuery(raw_query); // проверку на минусы и валидность закинул в ParseQueryWord
        std::vector<Document> matched_documents = FindAllDocuments(query_words, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs)
        {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON)
                 return lhs.rating > rhs.rating;
            else
            {
                return lhs.relevance > rhs.relevance;
            }
        });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const // задан статус
    {
        return FindTopDocuments(raw_query, [status](int id, DocumentStatus stat, int rating){ return stat == status;});
    }

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const // дефолтный случай
    {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

private:
    struct DocumentInfo
    {
        int rating;
        DocumentStatus status;
    };

    struct Query
    {
        std::set<std::string> minus_words;
        std::set<std::string> plus_words;
    };

    std::vector<int> docs_id_;

    std::unordered_map<int, DocumentInfo> id_doc_info_;

    int document_count_ = 0;

    std::map<std::string, std::map<int, double>> word_to_document_id_freqs_;

    const std::set<std::string> stop_words_;

    void CheckIsValidAndMinuses(const std::string& query_word) const
    {
        if (DetectTwoMinus(query_word))
        {
            throw std::invalid_argument("Two minuses before one word");
        }

        if (DetectNoWordAfterMinus(query_word))
        {
            throw std::invalid_argument("No word after minus");
        }

        if (!IsValidWord(query_word))
        {
            throw std::invalid_argument("Query contains symbols with codes from 0 to 31");
        }
    }

    template<typename Collection>
    void SetStopWords(const Collection& collection)
    {
        for (const auto& word : collection)
        {
            stop_words_.insert(word);
        }
    }

    static int ComputeAverageRating(const std::vector<int>& ratings)
    {
        int average_rating = std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        return average_rating;
    }

    bool IsStopWord(const std::string& word) const
    {
        return stop_words_.count(word) > 0;
    }

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const
    {
        std::vector<std::string> words;
        for (const std::string& word : SplitIntoWords(text))
        {
            if (!IsValidWord(word))
            {
                const std::string hint = "Invalid word: " + word;
                throw std::invalid_argument(hint);
            }

            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    void ParseQueryWord(const std::string& word, Query& query_words) const
    {
        CheckIsValidAndMinuses(word);

        if (word[0] == '-') // минус-слово
        {
            std::string no_minus = word.substr(1);
            if (!IsStopWord(no_minus))
            {
                query_words.minus_words.insert(no_minus);
            }
        }
        else // плюс-слово
        {
            query_words.plus_words.insert(word);
        }
    }

    static bool DetectTwoMinus(const std::string& query_word)
    {
        return query_word.substr(0, 2) == "--";
    }

    static bool DetectNoWordAfterMinus(const std::string& query_word)
    {
        return query_word == "-";
    }

    static bool IsValidWord(const std::string& query_word)
    {
        return none_of(query_word.begin(), query_word.end(), [](char c)
        {
            return c >= '\0' && c < ' ';
        });
    }

    Query ParseQuery(const std::string& text) const
    {
        Query query_words;
        for (std::string& word : SplitIntoWordsNoStop(text))
        {
            ParseQueryWord(word, query_words);
        }

        // если минус-слово и плюс-слово одинаковы -> убираем плюс-слово
        for (const std::string& minus_word : query_words.minus_words)
        {
            if (query_words.plus_words.count(minus_word) != 0)
            {
                query_words.plus_words.erase(minus_word);
            }
        }
        return query_words;
    }

    inline double ComputeIDF(const std::string& plus_word) const
    {
        return log(document_count_ * 1.0 / word_to_document_id_freqs_.at(plus_word).size());
    }

    template <typename T>
    std::vector<Document> FindAllDocuments(const Query& query_words, T predicate) const
    {
        std::map<int, double> document_to_relevance;
        std::vector<Document> matched_documents;

        for (const std::string& plus_word : query_words.plus_words) // итерируем по плюс-словам
        {
            if (word_to_document_id_freqs_.find(plus_word) != word_to_document_id_freqs_.end()) // нашли плюс-слово
            {
                double IDF = ComputeIDF(plus_word);
                for (const auto& [id, TF] : word_to_document_id_freqs_.at(plus_word)) // итерируем по документам
                {
                    document_to_relevance[id] += IDF * TF; // аккумулируем TF * IDF
                }
            }
        }

        for (const std::string& minus_word : query_words.minus_words) // тоже самое, что и сверху только теперь удаляем минус-слова
        {
            if (word_to_document_id_freqs_.find(minus_word) != word_to_document_id_freqs_.end())
            {
                for (const auto& [id, TF] : word_to_document_id_freqs_.at(minus_word))
                {
                    document_to_relevance.erase(id);
                }
            }
        }

        for (const auto& [document_id, relevance] : document_to_relevance)
        {
            if (predicate(document_id, id_doc_info_.at(document_id).status, id_doc_info_.at(document_id).rating)) // предикат возвращает булево значение
            {
                matched_documents.push_back({document_id, relevance, id_doc_info_.at(document_id).rating});
            }
        }
        return matched_documents;
    }

};
