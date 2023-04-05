#include "document.h"

std::ostream& operator<<(std::ostream& out, const Document& doc)
{
    using namespace std::literals;
    out << "{ document_id = "s << doc.id << ", relevance = "s << doc.relevance << ", rating = "s << doc.rating << " }"s;
    return out;
}
