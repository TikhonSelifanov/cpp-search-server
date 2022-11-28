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
            word_to_document_freqs_[word][document_id] += 1 * 1.0 / words.size();
        }
        ++_documentCount;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus stat = DocumentStatus::ACTUAL) const
    {
        const Query query_words = ParseQuery(raw_query);
        vector<Document> matched_documents = FindAllDocuments(query_words, stat);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs)
        {
                 return lhs.relevance > rhs.relevance;
        });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
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

    map<string, map<int, double>> word_to_document_freqs_;

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

    Query ParseQuery(const string& text) const
    {
        Query queryWords;
        for (string& word : SplitIntoWordsNoStop(text))
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
        // если есть минус-слово и плюс-слово одинаковы -> убираем плюс-слово
        for (const string& minusWord : queryWords.minusWords)
        {
            if (queryWords.plusWords.count(minusWord) != 0)
            {
                queryWords.plusWords.erase(minusWord);
            }
        }
        return queryWords;
    }

    vector<Document> FindAllDocuments(const Query& query_words, const DocumentStatus& stat) const
    {
        map<int, double> document_to_relevance;
        vector<Document> matched_documents;

        for (const string& plusWord : query_words.plusWords) // итерируем по плюс-словам
        {
            if (word_to_document_freqs_.find(plusWord) != word_to_document_freqs_.end()) // нашли плюс-слово
            {
                double IDF = log(_documentCount * 1.0 / word_to_document_freqs_.at(plusWord).size());
                for (const pair<int, double>& docIdAndTF : word_to_document_freqs_.at(plusWord)) // итерируем по документам
                {
                    document_to_relevance[docIdAndTF.first] += IDF * docIdAndTF.second; // аккумулируем TF * IDF
                }
            }
        }

        for (const string& minusWord : query_words.minusWords) // тоже самое, что и сверху только теперь удаляем минус-слова
        {
            if (word_to_document_freqs_.find(minusWord) != word_to_document_freqs_.end())
            {
                for (const pair<int, double>& docIdAndTF : word_to_document_freqs_.at(minusWord))
                {
                    document_to_relevance.erase(docIdAndTF.first);
                }
            }
        }

        for (const auto& [document_id, relevance] : document_to_relevance)
        {
            if (_idAndDocumentInfo.at(document_id) == stat)
                matched_documents.push_back({document_id, relevance, _documentRatings.at(document_id)});
        }
        return matched_documents;
    }

};

//SearchServer CreateSearchServer()
//{
//    SearchServer search_server;
//    search_server.SetStopWords(ReadLine());

//    const int document_count = ReadLineWithNumber();
//    for (int document_id = 0; document_id < document_count; ++document_id)
//    {
//        const string document = ReadLine();
//        const vector<int> ratings = ReadLineWithRatings();
//        search_server.AddDocument(document_id, document, DocumentStatus,ratings);
//        ReadLine();
//    }

//    return search_server;
//}

//int main()
//{
//    const SearchServer search_server = CreateSearchServer();

//    const string query = ReadLine();
//    for (const auto& [document_id, relevance, rating] : search_server.FindTopDocuments(query))
//    {
//        cout << "{ document_id = "s << document_id << ", " << "relevance = "s << relevance << ", " << "rating = " << rating << " }"s << endl;
//    }
//}
