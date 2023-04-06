#include "search_server.h"

using namespace std::literals;

int SearchServer::GetDocumentCount() const
{
    return docs_id_.size();
}

const std::set<int>::iterator SearchServer::begin()
{
    return docs_id_.begin();
}

const std::set<int>::iterator SearchServer::end()
{
    return docs_id_.end();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    static const std::map<std::string, double> empty = {};
    if (id_to_word_freqs_.count(document_id) != 0)
    {
        return id_to_word_freqs_.at(document_id);
    }
    else
    {
        return empty;
    }
}

void SearchServer::RemoveDocument(int document_id)
{
    for (auto& [word, freqs] : id_to_word_freqs_.at(document_id))
    {
        word_to_document_id_freqs_.at(word).erase(document_id);
        if (word_to_document_id_freqs_.at(word).empty())
        {
            word_to_document_id_freqs_.erase(word);
        }
    }
    id_doc_info_.erase(document_id);
    id_to_word_freqs_.erase(document_id);
    for (auto it = docs_id_.begin(); it != docs_id_.end(); ++it)
    {
        if (*it == document_id)
        {
            docs_id_.erase(it);
            break;
        }
    }
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const // задан статус
{
    return FindTopDocuments(raw_query, [status](int id, DocumentStatus stat, int rating){ return stat == status;});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const // дефолтный случай
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

void SearchServer::CheckIsValidAndMinuses(const std::string& query_word) const
{
    if (DetectTwoMinus(query_word))
    {
        throw std::invalid_argument("Two minuses before one word"s);
    }

    if (DetectNoWordAfterMinus(query_word))
    {
        throw std::invalid_argument("No word after minus"s);
    }

    if (!IsValidWord(query_word))
    {
        throw std::invalid_argument("Query contains symbols with codes from 0 to 31"s);
    }
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
    int average_rating = std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    return average_rating;
}

bool SearchServer::IsStopWord(const std::string& word) const
{
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const
{
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text))
    {
        if (!IsValidWord(word))
        {
            const std::string hint = "Invalid word: "s + word;
            throw std::invalid_argument(hint);
        }

        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::Query SearchServer::ParseQueryWord(const std::string& word) const
{
    Query query_words;
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
    return query_words;
}

bool SearchServer::DetectTwoMinus(const std::string& query_word)
{
    return query_word.substr(0, 2) == "--";
}

bool SearchServer::DetectNoWordAfterMinus(const std::string& query_word)
{
    return query_word == "-";
}

bool SearchServer::IsValidWord(const std::string& query_word)
{
    return none_of(query_word.begin(), query_word.end(), [](char c)
    {
        return c >= '\0' && c < ' ';
    });
}
void SearchServer::Query::insert(Query query_words)
{
    minus_words.merge(query_words.minus_words);
    plus_words.merge(query_words.plus_words);
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const
{
    Query query_words;
    for (const std::string& word : SplitIntoWordsNoStop(text))
    {
        query_words.insert(ParseQueryWord(word));
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

double SearchServer::ComputeIDF(const std::string& plus_word) const
{
    return log(docs_id_.size() * 1.0 / word_to_document_id_freqs_.at(plus_word).size());
}

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
        throw std::invalid_argument("Id is less than 0"s);
    }

    if (id_doc_info_.find(document_id) != id_doc_info_.end())
    {
        throw std::invalid_argument("Document with such an id already exists"s);
    }

    docs_id_.insert(document_id);

    std::vector<std::string> words = SplitIntoWordsNoStop(document);

    int averageRating = ComputeAverageRating(ratings);
    id_doc_info_[document_id].rating = averageRating;

    id_doc_info_[document_id].status = stat;

    for (const std::string& word : words)
    {
        // аккумулируем TF для всех слов
        word_to_document_id_freqs_[word][document_id] += 1.0 / words.size();
        id_to_word_freqs_[document_id][word] += 1.0 / words.size();
    }
}
