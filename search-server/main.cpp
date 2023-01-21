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
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
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
        for (const string& word : stop_words_)
        {
            if (!IsValidWord(word))
            {
                throw invalid_argument("Word contains symbols with codes from 0 to 31"s);
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {

    }

    int GetDocumentCount() const
    {
        return _documentCount;
    }

    int GetDocumentId(int index) const
    {
        if (index >= 0 && index < _documentCount)
        {
            int res = 0;
            for (const auto& [id, stat] : _idAndDocumentInfo)
            {
                if (res == index)
                {
                    res = id;
                    return res;
                }
                ++res;
            }
        }
        throw out_of_range("Id is out of range"s);
        return SearchServer::INVALID_DOCUMENT_ID;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
    {

        if (DetectTwoMinus(raw_query))
        {
            throw invalid_argument("Tho minuses before one word"s);
        }

        if (DetectNoWordAfterMinus(raw_query))
        {
            throw invalid_argument("No word after minus"s);
        }

        if (!IsValidWord(raw_query))
        {
            throw invalid_argument("Query contains symbols with codes from 0 to 31"s);
        }

        vector<string> plusWords;
        Query queryWords = ParseQuery(raw_query);

        for (const string& minusWord : queryWords.minusWords)
        {
            if (word_to_documentId_freqs_.find(minusWord) != word_to_documentId_freqs_.end())
            {
                if (word_to_documentId_freqs_.at(minusWord).find(document_id) != word_to_documentId_freqs_.at(minusWord).end())
                {
                    return tuple(vector<string> {}, _idAndDocumentInfo.at(document_id));
                }
            }
        }

        for (const string& plusWord : queryWords.plusWords)
        {
            if (word_to_documentId_freqs_.find(plusWord) != word_to_documentId_freqs_.end())
            {
                if (word_to_documentId_freqs_.at(plusWord).find(document_id) != word_to_documentId_freqs_.at(plusWord).end())
                {
                    plusWords.push_back(plusWord);
                }
            }
        }
        return tuple(plusWords, _idAndDocumentInfo.at(document_id));
    }

    void AddDocument(const int document_id, const string& document, const DocumentStatus& stat, const vector<int>& ratings)
    {
        if (document_id < 0)
        {
            throw invalid_argument("Id is less than 0"s);
        }

        if (_idAndDocumentInfo.find(document_id) != _idAndDocumentInfo.end())
        {
            throw invalid_argument("Document with such an id already exists"s);
        }

        if (!IsValidWord(document))
        {
            throw invalid_argument("Document contains symbols with codes from 0 to 31"s);
        }

        vector<string> words = SplitIntoWordsNoStop(document);

        int averageRating = ComputeAverageRating(ratings);
        _documentRatings[document_id] = averageRating;

        _idAndDocumentInfo[document_id] = stat;

        for (string& word : words)
        {
            // аккумулируем TF для всех слов
            if (word[0] == '-')
            {
                word = word.substr(1);
            }
            word_to_documentId_freqs_[word][document_id] += 1 * 1.0 / words.size();
        }
        ++_documentCount;
    }

    template <typename T>
    vector<Document> FindTopDocuments(const string& raw_query, T predicat) const // задано условие
    {
        if (DetectTwoMinus(raw_query))
        {
            throw invalid_argument("Two minuses before one word"s);
        }

        if (DetectNoWordAfterMinus(raw_query))
        {
            throw invalid_argument("No word after minus"s);
        }

        if (!IsValidWord(raw_query))
        {
            throw invalid_argument("Query contains symbols with codes from 0 to 31"s);
        }

        const Query query_words = ParseQuery(raw_query);
        vector<Document> matched_documents = FindAllDocuments(query_words, predicat);

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
        set<string> minusWords;
        set<string> plusWords;
    };

    map<int, DocumentStatus> _idAndDocumentInfo;

    int _documentCount = 0;

    map<string, map<int, double>> word_to_documentId_freqs_;

    const set<string> stop_words_;

    map<int, int> _documentRatings;

    template<typename Collection>
    void _SetStopWords(const Collection& coll)
    {
        for (const auto& word : coll)
        {
            stop_words_.insert(word);
        }
    }

    static int ComputeAverageRating(const vector<int>& ratings)
    {
        int averageRating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        return averageRating;
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
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    void ParseQueryWord(const string& word, Query& queryWords) const
    {
        if (word[0] == '-') // минус-слово
        {
            string noMinus = word.substr(1);
            if (!IsStopWord(noMinus))
            {
                queryWords.minusWords.insert(noMinus);
            }
        }
        else // плюс-слово
        {
            queryWords.plusWords.insert(word);
        }
    }

    static bool DetectTwoMinus(const string& raw_query)
    {
        vector<string> words = SplitIntoWords(raw_query);
        for (const string& word : words)
        {
            if (word.substr(0, 2) == "--"s)
            {
                return true;
            }
        }
        return false;
    }

    static bool DetectNoWordAfterMinus(const string& raw_query)
    {
        vector<string> words = SplitIntoWords(raw_query);
        for (const string& word : words)
        {
            if (word == "-"s)
            {
                return true;
            }
        }
        return false;
    }

    static bool IsValidWord(const string& word)
    {
        return none_of(word.begin(), word.end(), [](char c)
        {
            return c >= '\0' && c < ' ';
        });
    }

    Query ParseQuery(const string& text) const
    {
        Query queryWords;
        for (string& word : SplitIntoWordsNoStop(text))
        {
            ParseQueryWord(word, queryWords);
        }

        // если минус-слово и плюс-слово одинаковы -> убираем плюс-слово
        for (const string& minusWord : queryWords.minusWords)
        {
            if (queryWords.plusWords.count(minusWord) != 0)
            {
                queryWords.plusWords.erase(minusWord);
            }
        }
        return queryWords;
    }

    template <typename T>
    vector<Document> FindAllDocuments(const Query& query_words, T predicat) const
    {
        map<int, double> document_to_relevance;
        vector<Document> matched_documents;

        for (const string& plusWord : query_words.plusWords) // итерируем по плюс-словам
        {
            if (word_to_documentId_freqs_.find(plusWord) != word_to_documentId_freqs_.end()) // нашли плюс-слово
            {
                double IDF = log(_documentCount * 1.0 / word_to_documentId_freqs_.at(plusWord).size());
                for (const pair<int, double>& docIdAndTF : word_to_documentId_freqs_.at(plusWord)) // итерируем по документам
                {
                    document_to_relevance[docIdAndTF.first] += IDF * docIdAndTF.second; // аккумулируем TF * IDF
                }
            }
        }

        for (const string& minusWord : query_words.minusWords) // тоже самое, что и сверху только теперь удаляем минус-слова
        {
            if (word_to_documentId_freqs_.find(minusWord) != word_to_documentId_freqs_.end())
            {
                for (const pair<int, double>& docIdAndTF : word_to_documentId_freqs_.at(minusWord))
                {
                    document_to_relevance.erase(docIdAndTF.first);
                }
            }
        }

        for (const auto& [document_id, relevance] : document_to_relevance)
        {
            if (predicat(document_id, _idAndDocumentInfo.at(document_id), _documentRatings.at(document_id))) // предикат возвращает булево значение
            {
                matched_documents.push_back({document_id, relevance, _documentRatings.at(document_id)});
            }
        }
        return matched_documents;
    }

};

