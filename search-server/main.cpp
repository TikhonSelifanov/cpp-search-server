#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

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

struct Document
{
    int id;
    double relevance;
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

    void AddDocument(const int document_id, const string& document)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);

        for (const string& word : words)
        {
            // аккумулируем TF для всех слов
            word_to_document_freqs_[word][document_id] += 1 * 1.0 / words.size();
        }
        ++_documentCount;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const
    {
        const Query query_words = ParseQuery(raw_query);
        vector<Document> matched_documents = FindAllDocuments(query_words);

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

    struct Query
    {
        set<string> minusWords;
        set<string> plusWords;
    };

    int _documentCount = 0;

    map<string, map<int, double>> word_to_document_freqs_;

    set<string> stop_words_;

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

    vector<Document> FindAllDocuments(const Query& query_words) const
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

        for (const pair<int, double>& x : document_to_relevance)
        {
            matched_documents.push_back({x.first, x.second});
        }
        return matched_documents;
    }

};

SearchServer CreateSearchServer()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id)
    {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main()
{
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query))
    {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}
