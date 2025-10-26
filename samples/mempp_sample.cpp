#include "Mempp/Mempp.h"

#include <iostream>

int main()
{
    std::cout << "Hello World!\n";

    StaticVector<int, 20> vec(3, 1);
    for (int i = 0; i < 10; ++i) {
        vec.emplace_back(i);
    }

    for (auto& e : vec) {
        std::cout << e << " ";
    }
    std::cout << std::endl;

    auto it = vec.begin();
    it++;
    it++;
    it++;
    it++;
    auto it2 = it;
    it2++;
    it2++;
    vec.erase(it, it2);

    for (auto& e : vec) {
        std::cout << e << " ";
    }
    std::cout << std::endl;

    return 0;
}