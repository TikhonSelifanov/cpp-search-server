#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server)
{
    using namespace std::literals;
    std::map<std::set<std::string>, int> set_words_to_id;
    std::vector<int> id_to_delete;
    for (const int document_id : search_server)
    {
        std::set<std::string> temp;
        for (const auto& [word, freq] : search_server.GetWordFrequencies(document_id))
        {
            temp.insert(word);
        }

        if (set_words_to_id.count(temp) != 0)
        {
            int id = set_words_to_id.at(temp);
            if (id > document_id)
            {
                id_to_delete.push_back(id);
                std::cout << "Found duplicate document id "s << id << "\n"s;
                set_words_to_id.at(temp) = document_id;
            }
            else
            {
                id_to_delete.push_back(document_id);
                std::cout << "Found duplicate document id "s << document_id << "\n"s;
            }
        }
        else
        {
            set_words_to_id.insert({temp, document_id});
        }
    }

    for (const int id : id_to_delete)
    {
        search_server.RemoveDocument(id);
    }
}
