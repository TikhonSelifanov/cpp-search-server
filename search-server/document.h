#pragma once
#include <string>
#include <iostream>

enum struct DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document
{
    Document() : id(0), relevance(0.0), rating(0){}
    Document(int Id, double Relevance, int Rating) : id(Id), relevance(Relevance), rating(Rating) {}

    int id;
    double relevance;
    int rating;
};

std::ostream& operator<<(std::ostream& out, const Document& doc);
