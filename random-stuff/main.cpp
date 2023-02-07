#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <set>

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

int main() {
    cout << "Test1"s << endl;
    set<int> test1 = {1, 1, 1, 2, 3, 4, 5, 5};
    PrintRange(test1.begin(), test1.end());
    cout << "Test2"s << endl;
    vector<int> test2 = {}; // пустой контейнер
    PrintRange(test2.begin(), test2.end());
    cout << "End of tests"s << endl;
}

