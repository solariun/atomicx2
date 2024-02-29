#include "atomicx/atomicx.h"

class th : public atomicx::thread {
public:
    th() : atomicx::thread(m_stack) {
        std::cout << "thread::thread() - constructing." << std::endl;
    }
    ~th() {
        std::cout << "thread::~thread() - destructing." << std::endl;
    }

    bool run() override {
        do
        {
            std::cout << this << ": thread::run() EXECUTING... (current:" << atomicx::g_ctx.current_thread() << std::endl;
        } while(yield(1000));

        return true;
    }

    const char* name() override 
    {
        return "thread_example";
    }

private:
    size_t m_stack[1024];
};

void printThreadInfo(th& thread) {
    for (auto th : thread)
        std::cout << "th::" << th->name() << ":" << th << std::endl;
}

int main() {
    
    th t1;
    th t2;
    th t3;

    {
        th t4;
        th t5;

        std::cout << "t1:" << (*(t1.begin())) << "->" << (*(t1.begin()))->next << std::endl;
        printThreadInfo(t5);
    }

    printThreadInfo(t1);

    atomicx::g_ctx.start();

    return 0;
}

