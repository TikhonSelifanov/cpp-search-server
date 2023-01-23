#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <unordered_map>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<int> ReadLineWithRatings()
{
    int ratings_size;
    cin >> ratings_size;

    vector<int> ratings(ratings_size, 0);

    for (int& rating : ratings)
    {
        cin >> rating;
    }
    return ratings;
}

vector<string> SplitIntoWords(const string& text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings)
{
    set<string> non_empty_strings;
    for (const string& str : strings)
    {
        if (!str.empty())
        {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum struct DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document
{
    Document() : id(0), relevance(0.0), rating(0){}
    Document(int Id, double Relevance, int Rating) : id(Id), relevance(Relevance), rating(Rating) {}

    int id;
    double relevance;
    int rating;
};
class SearchServer
{
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        if (any_of(stop_words_.begin(), stop_words_.end(), [](const string& word) {return !IsValidWord(word);}))
        {
            throw invalid_argument("Word contains symbols with codes from 0 to 31"s);
        }
    }

    explicit SearchServer(const string& stop_words_text)
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
            throw out_of_range("Id is out of range"s);
        }
        return docs_id_[index];
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
    {
        vector<string> plus_words;
        Query query_words = ParseQuery(raw_query);

        for (const string& minus_word : query_words.minus_words)
        {
            if (word_to_document_id_freqs_.find(minus_word) != word_to_document_id_freqs_.end())
            {
                if (word_to_document_id_freqs_.at(minus_word).find(document_id) != word_to_document_id_freqs_.at(minus_word).end())
                {
                    return {vector<string> {}, id_doc_info_.at(document_id).status};
                }
            }
        }

        for (const string& plus_word : query_words.plus_words)
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

    void AddDocument(const int document_id, const string& document, const DocumentStatus& stat, const vector<int>& ratings)
    {
        if (document_id < 0)
        {
            throw invalid_argument("Id is less than 0"s);
        }

        if (id_doc_info_.find(document_id) != id_doc_info_.end())
        {
            throw invalid_argument("Document with such an id already exists"s);
        }

        docs_id_.push_back(document_id);

        vector<string> words = SplitIntoWordsNoStop(document);

        int averageRating = ComputeAverageRating(ratings);
        id_doc_info_[document_id].rating = averageRating;

        id_doc_info_[document_id].status = stat;

        for (const string& word : words)
        {
            // аккумулируем TF для всех слов
            word_to_document_id_freqs_[word][document_id] += 1.0 / words.size();
        }
        ++document_count_;
    }

    template <typename T>
    vector<Document> FindTopDocuments(const string& raw_query, T predicate) const // задано условие
    {
        const Query query_words = ParseQuery(raw_query); // проверку на минусы и валидность закинул в ParseQueryWord
        vector<Document> matched_documents = FindAllDocuments(query_words, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs)
        {
            if (abs(lhs.relevance - rhs.relevance) < EPSILON)
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const // задан статус
    {
        return FindTopDocuments(raw_query, [status](int id, DocumentStatus stat, int rating){ return stat == status;});
    }

    vector<Document> FindTopDocuments(const string& raw_query) const // дефолтный случай
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
        set<string> minus_words;
        set<string> plus_words;
    };

    vector<int> docs_id_;

    unordered_map<int, DocumentInfo> id_doc_info_;

    int document_count_ = 0;

    map<string, map<int, double>> word_to_document_id_freqs_;

    const set<string> stop_words_;

    void CheckIsValidAndMinuses(const string& query_word) const
    {
        if (DetectTwoMinus(query_word))
        {
            throw invalid_argument("Two minuses before one word"s);
        }

        if (DetectNoWordAfterMinus(query_word))
        {
            throw invalid_argument("No word after minus"s);
        }

        if (!IsValidWord(query_word))
        {
            throw invalid_argument("Query contains symbols with codes from 0 to 31"s);
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

    static int ComputeAverageRating(const vector<int>& ratings)
    {
        int average_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        return average_rating;
    }

    bool IsStopWord(const string& word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text))
        {
            if (!IsValidWord(word))
            {
                const string hint = "Invalid word: "s + word;
                throw invalid_argument(hint);
            }

            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    void ParseQueryWord(const string& word, Query& query_words) const
    {
        CheckIsValidAndMinuses(word);

        if (word[0] == '-') // минус-слово
        {
            string no_minus = word.substr(1);
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

    static bool DetectTwoMinus(const string& query_word)
    {
        return query_word.substr(0, 2) == "--"s;
    }

    static bool DetectNoWordAfterMinus(const string& query_word)
    {
        return query_word == "-"s;
    }

    static bool IsValidWord(const string& query_word)
    {
        return none_of(query_word.begin(), query_word.end(), [](char c)
        {
            return c >= '\0' && c < ' ';
        });
    }

    Query ParseQuery(const string& text) const
    {
        Query query_words;
        for (string& word : SplitIntoWordsNoStop(text))
        {
            ParseQueryWord(word, query_words);
        }

        // если минус-слово и плюс-слово одинаковы -> убираем плюс-слово
        for (const string& minus_word : query_words.minus_words)
        {
            if (query_words.plus_words.count(minus_word) != 0)
            {
                query_words.plus_words.erase(minus_word);
            }
        }
        return query_words;
    }

    inline double ComputeIDF(const string& plus_word) const
    {
        return log(document_count_ * 1.0 / word_to_document_id_freqs_.at(plus_word).size());
    }

    template <typename T>
    vector<Document> FindAllDocuments(const Query& query_words, T predicate) const
    {
        map<int, double> document_to_relevance;
        vector<Document> matched_documents;

        for (const string& plus_word : query_words.plus_words) // итерируем по плюс-словам
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

        for (const string& minus_word : query_words.minus_words) // тоже самое, что и сверху только теперь удаляем минус-слова
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
