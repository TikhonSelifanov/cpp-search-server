#pragma once
#include <iostream>
#include <vector>

template <typename Iterator>
struct IteratorRange
{
    IteratorRange(Iterator begin, Iterator end) : left_(begin), right_(end)
    {

    }

    inline Iterator begin() const
    {
        return left_;
    }

    inline Iterator end() const
    {
        return right_;
    }

    inline size_t size() const
    {
        return distance(left_, right_);
    }
private:
    Iterator left_;
    Iterator right_;
};

template<typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& iter)
{
    for (auto it = iter.begin(); it != iter.end(); ++it)
    {
        out << *it;
    }
    return out;
}

template <typename Iterator>
struct Paginator
{
    inline auto begin() const
    {
        return vec_.begin();
    }

    inline auto end() const
    {
        return vec_.end();
    }
    Paginator(Iterator begin, Iterator end, size_t size) : begin_(begin), end_(end), size_(size)
    {
        for (auto it = begin; it < end; advance(it, size))
        {
            if (distance(it, end) <= size)
            {
                vec_.push_back(IteratorRange{it, end});
            }
            else
            {
                vec_.push_back(IteratorRange{it, it + size_});
            }
        }
    }
private:
    std::vector<IteratorRange<Iterator>> vec_;
    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size)
{
    return Paginator(begin(c), end(c), page_size);
}
