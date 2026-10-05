#ifndef PLANETARIUM_CLIENT_LIFECYCLE_HPP
#define PLANETARIUM_CLIENT_LIFECYCLE_HPP
#include <map>
#include <memory>

// The map exposes raw pointers to legacy renderer code, but removal owns both
// allocations until their destructors run. Remove the entry before destruction.
template<class ClientType, class DataType>
void eraseOwnedClient(std::map<int, ClientType*>& clients, int id) {
    const auto it = clients.find(id);
    if (it == clients.end()) return;
    std::unique_ptr<ClientType> client(it->second);
    clients.erase(it);
    std::unique_ptr<DataType> data(static_cast<DataType*>(client->extra));
    client->extra = nullptr;
}
#endif
