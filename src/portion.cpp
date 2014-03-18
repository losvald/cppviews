#include "portion.hpp"

Portion::Portion(void* begin, size_t size)  : begin_(begin), size_(size) {}

Portion::Portion(size_t size, void* end)  : begin_(end - size), size_(size) {}
