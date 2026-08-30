#include <iostream>
#include <deque>

int main()
{
    std::deque<int> Q = {10, 20, 30};

    Q.push_back(40);
    Q.pop_back();  

    std::cout << Q.front() << std::endl;
    std::cout << Q.back() << std::endl;

    for (int num : Q) {
        std::cout << num << ", "; 
    }

    return 0;
}