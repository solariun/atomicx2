#include "atomicx/atomicx.h"

class th : public atomicx::thread {
public:
    th() : atomicx::thread(m_stack) {
        std::cout << "thread::thread()" << std::endl;
    }
    ~th() {
        std::cout << "thread::~thread()" << std::endl;
    }

    bool run() override {
        std::cout << "thread::run()" << std::endl;
        return true;
    }

    size_t m_stack[1024];
};

int main() {
    
    th t1;
    th t2;
    th t3;
    {
        th t4;
        th t5;

        std::cout << "t1:" << (*(t1.begin())) << "->" << (*(t1.begin()))->next << std::endl;
        for (auto th : t5)
            std::cout << "th:" << th << std::endl;
    }

    for (auto th : t1)
        std::cout << "th:" << th << std::endl;

    return 0;
}

