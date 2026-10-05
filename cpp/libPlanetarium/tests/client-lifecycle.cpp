#include "../client-lifecycle.hpp"
#include <cassert>
#include <iostream>

struct Data {
    static int live;
    std::unique_ptr<int[]> resource = std::make_unique<int[]>(1024);
    Data() { ++live; }
    ~Data() { --live; }
};
int Data::live = 0;

struct Client {
    static int live;
    void* extra = nullptr;
    Client() { ++live; }
    ~Client() { assert(extra == nullptr); --live; }
};
int Client::live = 0;

int main() {
    std::map<int, Client*> clients;
    auto add = [&](int id) {
        eraseOwnedClient<Client, Data>(clients, id);
        auto client = std::make_unique<Client>();
        auto data = std::make_unique<Data>();
        client->extra = data.get();
        clients.emplace(id, client.get());
        data.release(); client.release();
    };
    add(100);
    Client* observer = clients.at(100);
    for (int i = 1; i <= 1000; ++i) {
        add(-i);
        eraseOwnedClient<Client, Data>(clients, -i);
        eraseOwnedClient<Client, Data>(clients, -i); // idempotent
        assert(clients.at(100) == observer);
        assert(Client::live == 1 && Data::live == 1);
    }
    add(100); // replacing a live client releases its old resources
    assert(Client::live == 1 && Data::live == 1);
    add(0);
    while (!clients.empty()) eraseOwnedClient<Client, Data>(clients, clients.begin()->first);
    assert(Client::live == 0 && Data::live == 0);
    std::cout << "Client lifecycle: 1000 temporary clients, replacement and cleanup OK\n";
}
