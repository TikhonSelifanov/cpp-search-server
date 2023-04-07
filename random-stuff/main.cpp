#include <iostream>
#include <new>
#include <string>

using namespace std;

int main()
{
    size_t n = 0;
    try
    {
        // Так создаётся бесконечный цикл.
        for (;;)
        {
            // Внимание! Эта программа ведёт себя некорректно в иллюстративных целях.
            // Указатель на выделенную в куче строку никуда не сохраняется, что
            // приводит к утечке памяти.
            new string(100'000'000, ' ');
            ++n;
        }
    }
    catch (const bad_alloc&)
    {
        // Рано или поздно доступная программе память закончится,
        // и оператор new выбросит исключение bad_alloc
        cout << "bad_alloc after "s << n << " allocations"s << endl;
    }
}
