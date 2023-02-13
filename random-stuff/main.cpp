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

template<typename Cont, typename Elem>
void FindAndPrint(Cont& cont, const Elem& el)
{
    auto iter = find_if(cont.begin(), cont.end(), [el](const Elem& elem)
    {
        return elem == el;
    });
    PrintRange(cont.begin(), iter);
    PrintRange(iter, cont.end());
}

int main() {
    set<int> test = {1, 1, 1, 2, 3, 4, 5, 5};
    cout << "Test1"s << endl;
    FindAndPrint(test, 3);
    cout << "Test2"s << endl;
    FindAndPrint(test, 0); // элемента 0 нет в контейнере
    cout << "End of tests"s << endl;
}
