#include <numeric>

#include "search_server.h"

using namespace std;


// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов

template <typename T>
ostream& operator<<(ostream& os, const vector<T> vec)
{
    os << "["s;
    bool isFirst = true;
    for (const auto& x : vec)
    {
        if (!isFirst)
        {
            os << ", "s;
        }
        isFirst = false;
        os << x;
    }
    os << "]"s;
    return os;
}

template <typename T>
ostream& operator<<(ostream& os, const set<T> set1)
{
    os << "{"s;
    bool isFirst = true;
    for (const auto& x : set1)
    {
        if (!isFirst)
        {
            os << ", "s;
        }
        isFirst = false;
        os << x;
    }
    os << "}"s;
    return os;
}

template <typename T, typename U>
ostream& operator<<(ostream& os, const map<T, U> map1)
{
    os << "{"s;
    bool isFirst = true;
    for (const auto& [key, value] : map1)
    {
        if (!isFirst)
        {
            os << ", "s;
        }
        isFirst = false;
        os << key << ": " << value;
    }
    os << "}"s;
    return os;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u)
    {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty())
        {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

//#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

//#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

//#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

//#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        const string hint1 = "You probably forgot to increase document counter or you lost this doc somewhere in FindTopDocuments method."s;
        ASSERT_HINT(found_docs.size() == 1, hint1);
        const Document& doc0 = found_docs[0];
        const string hint2 = "Did you save the id number correctly?"s;
        ASSERT_EQUAL_HINT(doc0.id, doc_id, hint2);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Check the correctness of SetStopWords method."s);
    }
}

void TestExcludeMinusWords()
{
    const int docId = 1;
    const string content = "dog in the park"s;
    const vector<int> ratings = {3, 8, 5};

    SearchServer server;
    server.AddDocument(docId, content, DocumentStatus::ACTUAL, ratings);

    const string hint1 = "Should be non-empty."s;
    ASSERT_HINT(!server.FindTopDocuments("dog").empty(), hint1);

    const string hint2 = "Should be empty."s;
    ASSERT_HINT(server.FindTopDocuments("-dog"s).empty(), hint2);

}

void TestDocumentMatching()
{
    const int docId1 = 15;
    const string content1 = "blue bird in the park"s;
    const vector<int> ratings1 = {4, 2, 7, 5};

    const int docId2 = 3;
    const string content2 = "black mouse on the fridge"s;
    const vector<int> ratings2 = {2, 2, 2, 2};

    SearchServer server;
    server.AddDocument(docId1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(docId2, content2, DocumentStatus::ACTUAL, ratings2);

    const tuple testTuple = tuple(vector<string> {"blue"s, "park"s, "the"s}, DocumentStatus::ACTUAL);
    const string hint = "Check the part of the MatchDocument method where it adds plus words."s;
    ASSERT_HINT(server.MatchDocument("blue the park"s, docId1) == testTuple, hint);
}

void TestDescendingRelevanceOrder()
{
    const int docId1 = 15;
    const string content1 = "blue bird in the park"s;
    const vector<int> ratings1 = {4, 2, 7, 5};

    const int docId2 = 3;
    const string content2 = "black mouse on the fridge"s;
    const vector<int> ratings2 = {2, 2, 2, 2};

    const int docId3 = 17;
    const string content3 = "white cat in the room"s;
    const vector<int> ratings3 = {4, 2, 7, 5};

    SearchServer server;
    server.AddDocument(docId1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(docId2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(docId3, content3, DocumentStatus::ACTUAL, ratings3);

    const vector<Document> matched = server.FindTopDocuments("the in"s);
    vector<double> test1;
    vector<double> test2;
    for (const Document& doc : matched)
    {
        test1.push_back(doc.relevance);
    }

    for (const Document& doc : matched)
    {
        test2.push_back(doc.relevance);
    }
    sort(test1.begin(),test1.end(), [](const double& lhs, const double& rhs)
    {
        return lhs > rhs;
    });

    const string hint = "Check the sort function in the FindTopDocuments method."s;
    ASSERT_EQUAL_HINT(test2, test1, hint);

}

void TestFindAverageRating()
{
    const int docId1 = 15;
    const string content1 = "blue bird in the park"s;
    const vector<int> ratings1 = {4, 2, 7, 5};

    SearchServer server;
    server.AddDocument(docId1, content1, DocumentStatus::ACTUAL, ratings1);

    const int averageRating1 = accumulate(ratings1.begin(), ratings1.end(), 0) / static_cast<int>(ratings1.size());

    const vector<Document> test = server.FindTopDocuments("blue"s);
    const int testRating = test.front().rating;
    const string hint = "Check the correctness of finding the average value."s;
    ASSERT_EQUAL_HINT(averageRating1, testRating, hint);
}

void TestUserDefinedPredicate()
{
    const int docId1 = 1;
    const string content1 = "blue bird in the park"s;
    const vector<int> ratings1 = {4, 2, 7, 5};

    const int docId2 = 2;
    const string content2 = "black mouse on the fridge"s;
    const vector<int> ratings2 = {2, 2, 2, 2};

    const int docId3 = 3;
    const string content3 = "white cat in the room"s;
    const vector<int> ratings3 = {4, 2, 8, 8};

    SearchServer server;
    server.AddDocument(docId1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(docId2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(docId3, content3, DocumentStatus::REMOVED, ratings3);

    auto predicate1 = [](int id, DocumentStatus status, int rating) {return rating <= 3;};

    auto predicate2 = [](int id, DocumentStatus status, int rating) {return status == DocumentStatus::REMOVED;};

    auto predicate3 = [](int id, DocumentStatus status, int rating) {return id <= 2;};

    const string hint = "Check the FindAllDocuments private method. Does it work correctly with the custom predicate?"s;

    ASSERT_EQUAL_HINT(server.FindTopDocuments("the"s, predicate1).front().id, docId2, hint);

    ASSERT_EQUAL_HINT(server.FindTopDocuments("in"s, predicate2).front().id, docId3, hint);

    ASSERT_EQUAL_HINT(server.FindTopDocuments("the"s, predicate3).front().rating, 4, hint);
}

void TestUserDefinedStatus()
{
    const int docId1 = 1;
    const string content1 = "blue bird in the park"s;
    const vector<int> ratings1 = {4, 2, 7, 5};

    const int docId2 = 2;
    const string content2 = "black mouse on the fridge"s;
    const vector<int> ratings2 = {2, 2, 2, 2};

    const int docId3 = 3;
    const string content3 = "white cat in the room"s;
    const vector<int> ratings3 = {4, 2, 8, 8};

    SearchServer server;
    server.AddDocument(docId1, content1, DocumentStatus::REMOVED, ratings1);
    server.AddDocument(docId2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(docId3, content3, DocumentStatus::REMOVED, ratings3);

    const vector<Document> test = server.FindTopDocuments("the"s, DocumentStatus::REMOVED);
    const string hint = "Check the FindTopDocuments method."s;
    ASSERT_EQUAL_HINT(test.size(), 2, hint);
}

void TestComputeCorrectRelevance()
{
    const int docId1 = 1;
    const string content1 = "blue bird in the park"s;
    const vector<int> ratings1 = {4, 2, 7, 5};

    SearchServer server;
    server.AddDocument(docId1, content1, DocumentStatus::ACTUAL, ratings1);

    const double IDF = log(1);
    const double TF = 0.2;

    const string hint = "Check the correctness of computing the TF-IDF (Probably you should check the FindAllDocuments method and the AddDocument method)."s;

    ASSERT_EQUAL_HINT(server.FindTopDocuments("the"s).front().relevance, IDF * TF, hint);
}


/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer()
{
    TestExcludeStopWordsFromAddedDocumentContent();
    TestExcludeMinusWords();
    TestDocumentMatching();
    TestDescendingRelevanceOrder();
    TestFindAverageRating();
    TestUserDefinedPredicate();
    TestUserDefinedStatus();
    TestComputeCorrectRelevance();
    // Не забудьте вызывать остальные тесты здесь
}