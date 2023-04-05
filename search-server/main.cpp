#include "paginator.h"
#include "request_queue.h"
#include "remove_duplicates.h"

using namespace std;

int main() {
    SearchServer search_server(std::string("i v na"));

    search_server.AddDocument(1, "pyshistyi kot pyshistyi hvost", DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "pyshistyi pes i modnyi osheinik", DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "bolshoy kot modnyi osheinik ", DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "bolshoy pes skvorets evgeniy", DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "bolshoy pes skvorets vasiliy", DocumentStatus::ACTUAL, {1, 1, 1});

    const auto search_results = search_server.FindTopDocuments("pyshistyi pes");
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);

    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        std::cout << *page << std::endl;
        std::cout << "Разрыв страницы" << std::endl;
    }
}
