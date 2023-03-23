#include "read_input_functions.h"

std::string ReadLine()
{
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result = 0;
    std::cin >> result;
    ReadLine();
    return result;
}

std::vector<int> ReadLineWithRatings()
{
    int ratings_size;
    std::cin >> ratings_size;

    std::vector<int> ratings(ratings_size, 0);

    for (int& rating : ratings)
    {
        std::cin >> rating;
    }
    return ratings;
}
