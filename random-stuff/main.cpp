#include <iostream>
#include <numeric>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

template<typename T>
void PrintRange(T begin, T end)
{
    bool isFirst = true;
    for (auto x = begin; x != end; ++x)
    {
        if (isFirst)
        {
            cout << *x;
            isFirst = false;
            continue;
        }
        cout << ' ' << *x;
    }
    cout << endl;
}

template<typename Iter>
void Merge(Iter beg, Iter mid, Iter end)
{
    std::vector<typename Iter::value_type> temp;

    Iter a;
    Iter left = beg;
    Iter right = mid;
    while (left != mid && right != end)
    {
        if (*right < *left)
        {
            temp.emplace_back(*right++);
        }
        else
        {
            temp.emplace_back(*left++);
        }
    }
    temp.insert(temp.end(), left, mid);
    temp.insert(temp.end(), right, end);

    std::move(temp.begin(), temp.end(), beg);
}

template <typename RandomIt>
void MergeSort(RandomIt range_begin, RandomIt range_end)
{
    int size = distance(range_begin, range_end);
    auto mid = next(range_begin, size / 2);

    if (size <= 1)
    {
        return;
    }

    MergeSort(range_begin, mid);
    MergeSort(mid, range_end);
    Merge(range_begin, mid, range_end);
}

int main() {
    vector<int> test_vector = {5, 2, 3, 1};
    // iota             -> http://ru.cppreference.com/w/cpp/algorithm/iota
    // Заполняет диапазон последовательно возрастающими значениями

    // Выводим вектор до сортировки
    PrintRange(test_vector.begin(), test_vector.end());
    // Сортируем вектор с помощью сортировки слиянием
    MergeSort(test_vector.begin(), test_vector.end());
    // Выводим результат
    PrintRange(test_vector.begin(), test_vector.end());
    return 0;
}
