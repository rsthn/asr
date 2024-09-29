#include <iostream>
#include <string>
#include <vector>

#include <asr/ptr>

using namespace asr;
using namespace std;

class Being
{
    public:

    string name;
    int birthyear;

    Being(string name, int birthyear) : name(name), birthyear(birthyear) { }
    virtual ~Being() { }

    virtual void print() {
        cout << "[Being] " << name << " " << birthyear << endl;
    }
};

class Person : public Being
{
    public:

    Person(string name, int birthyear) : Being(name, birthyear)
    { }

    void print() override {
        cout << "[Person] " << name << " " << birthyear << endl;
    }
};

vector<ptr<Being>> getList()
{
    vector<ptr<Being>> list;

    ptr<Being> x = new Being("The Thing", 1982);
    ptr<Being> y = new Person("Jon", 1988);
    auto dummy = ptr<Being>(new Person("Jane", 1993));

    list.push_back(x);
    list.push_back(y);
    return list;
}

void test() {
    for (auto &item : getList())
        item->print();
}

int main (int argc, const char *argv[])
{
    auto n = asr::memblocks;

    test();

    asr::refs::shutdown();
    if (asr::memblocks != n)
        cout << "\e[31mMemory leak detected: \e[91m" << asr::memsize << " bytes\e[0m\n";

    return 0;
}
