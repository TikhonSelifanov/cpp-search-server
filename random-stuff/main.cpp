#include <iostream>
#include <stdlib.h>
#include <stdexcept>
#include <vector>

using namespace std;

class Tower
{
public:
    // конструктор и метод SetDisks нужны, чтобы правильно создать башни
    Tower(int disks_num)
    {
        FillTower(disks_num);
    }

    int GetDisksNum() const
    {
        return disks_.size();
    }

    void SetDisks(int disks_num)
    {
        FillTower(disks_num);
    }

    // добавляем диск на верх собственной башни
    // обратите внимание на исключение, которое выбрасывается этим методом
    void AddToTop(int disk)
    {
        int top_disk_num = disks_.size() - 1;
        if (0 != disks_.size() && disk >= disks_[top_disk_num])
        {
            throw invalid_argument("Невозможно поместить большой диск на маленький");
        }
        else
        {
            disks_.push_back(disk);
        }
    }
    // disks_num - количество перемещаемых дисков
    // destination - конечная башня для перемещения
    // buffer - башня, которую нужно использовать в качестве буфера для дисков
    void MoveDisks(int disks_num, Tower& destination, Tower& buffer)
    {
        if (disks_num == 0)
        {
            return;
        }
        MoveDisks(disks_num - 1, buffer, destination);

        destination.AddToTop(disks_.back());
        disks_.pop_back();

        buffer.MoveDisks(disks_num - 1, destination, *this);
    }
private:
    vector<int> disks_;

    // используем приватный метод FillTower, чтобы избежать дубликации кода
    void FillTower(int disks_num)
    {
        for (int i = disks_num; i > 0; i--)
        {
            disks_.push_back(i);
        }
    }
};

void SolveHanoi(vector<Tower>& towers) {
    int disks_num = towers[0].GetDisksNum();
    // запускаем рекурсию
    // просим переложить все диски на последнюю башню
    // с использованием средней башни как буфера
    towers[0].MoveDisks(disks_num, towers[2], towers[1]);
}
