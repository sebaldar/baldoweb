#ifndef __tmysql__
#define __tmysql__

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <mariadb/mysql.h>

class TConnection {
    friend class TQuery;

    class TActive {
        TConnection* connection;

    public:
        explicit TActive(TConnection* _con) : connection(_con) {}
        TActive& operator=(bool connect);
        operator bool() const { return connection->fConnected; }
    };

    class TNAME {
        TConnection* connection;

    public:
        enum NAME_TYPE { HOST, USER, DB, PASSWORD } type;
        TNAME(TConnection* _con, NAME_TYPE a_type);
        TNAME& operator=(const std::string& name);
        operator std::string() const;
    };

    std::string fHost;
    std::string fUser;
    std::string fDb;
    std::string fPassword;
    bool fConnected;

public:
    MYSQL _mysql;

    TConnection();
    TConnection(std::string_view host,
                std::string_view user,
                std::string_view password,
                std::string_view db);
    ~TConnection();

    TActive Active;
    TNAME Host, User, Db, Password;

    void Open();
    void Close();
};

class TQuery {
protected:
    TConnection* fConnection = nullptr;
    bool isConnectDirect = false;
    MYSQL* mysql = nullptr;
    int current_record = 0;
    MYSQL_ROW _row = nullptr;
    MYSQL_RES* _res = nullptr;
    bool fIsOpen = false;

public:
    int fRecord = 0;
    std::vector<std::string> SQL;

    TQuery() = default;
    explicit TQuery(TConnection& connection);
    explicit TQuery(TConnection* connection);
    TQuery(std::string_view host, std::string_view user,
           std::string_view password, std::string_view db);
    ~TQuery();

    TQuery& operator=(TConnection& connection);

    [[nodiscard]] bool run_query_with_retry(std::string_view query,
                                            int max_retries,
                                            unsigned int& last_errno,
                                            std::string& last_error);

    [[nodiscard]] std::string FieldByName(std::string_view nome_campo) const;

    void Ping() { mysql_ping(mysql); }

    void Open(std::string_view query);
    void Open();
    void Close();

    void First();
    void Next();
    void Prior();
    void MoveBy(int n);

    [[nodiscard]] bool Eof() const { return current_record == fRecord; }
    [[nodiscard]] bool Bof() const { return current_record == 0; }
    [[nodiscard]] int currentRecord() const { return current_record; }
    [[nodiscard]] bool isEmpty() const { return Eof() && Bof(); }

    void Add(std::string_view str);
};

// funzione di utilità per cancellazioni batch sicure
[[nodiscard]] bool delete_tipoP_in_batches(TQuery& q,
                                           size_t batch_size = 1000,
                                           int max_retries = 5);

#endif
