#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>

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

enum struct DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document
{
    int id;
    double relevance;
    int rating;
};

class SearchServer
{
public:

    int GetDocumentCount() const
    {
        return _documentCount;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
    {
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

    void SetStopWords(const string& text)
    {
        for (const string& word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    void AddDocument(const int document_id, const string& document, const DocumentStatus& stat, const vector<int>& ratings)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);

        int averageRating = ComputeAverageRating(ratings);
        _documentRatings[document_id] = averageRating;

        _idAndDocumentInfo[document_id] = stat;

        for (const string& word : words)
        {
            // аккумулируем TF для всех слов
            word_to_documentId_freqs_[word][document_id] += 1 * 1.0 / words.size();
        }
        ++_documentCount;
    }

    template <typename T>
    vector<Document> FindTopDocuments(const string& raw_query, T predicat) const // задано условие
    {
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

    set<string> stop_words_;

    map<int, int> _documentRatings;

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

    void ParseQueryWord(const string& word, Query queryWords) const
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

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}
int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "ACTUAL:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; })) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}
