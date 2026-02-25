#include "tmysql.hpp"
#include <sstream>
#include <thread>
#include <chrono>
#include <random>

// ========================= TActive =========================
TConnection::TActive& TConnection::TActive::operator=(bool connect) {
    if (!connection->fConnected && connect) connection->Open();
    else if (connection->fConnected && !connect) connection->Close();
    return *this;
}

// ========================= TNAME =========================
TConnection::TNAME::TNAME(TConnection* _con, NAME_TYPE a_type) : connection(_con), type(a_type) {}
TConnection::TNAME& TConnection::TNAME::operator=(const std::string& name) {
    switch (type) {
        case HOST: connection->fHost = name; break;
        case USER: connection->fUser = name; break;
        case DB: connection->fDb = name; break;
        case PASSWORD: connection->fPassword = name; break;
    }
    return *this;
}
TConnection::TNAME::operator std::string() const {
    switch (type) {
        case HOST: return connection->fHost;
        case USER: return connection->fUser;
        case DB: return connection->fDb;
        case PASSWORD: return connection->fPassword;
    }
    return "";
}

// ========================= TConnection =========================
TConnection::TConnection() :
    fHost(""), fUser(""), fDb(""), fPassword(""), fConnected(false),
    Active(this), Host(this, TNAME::HOST), User(this, TNAME::USER),
    Db(this, TNAME::DB), Password(this, TNAME::PASSWORD) {}

TConnection::TConnection(std::string_view host, std::string_view user,
                         std::string_view password, std::string_view db) :
    fHost(host), fUser(user), fDb(db), fPassword(password),
    fConnected(false), Active(this), Host(this, TNAME::HOST),
    User(this, TNAME::USER), Db(this, TNAME::DB), Password(this, TNAME::PASSWORD)
{
    Open();
}

TConnection::~TConnection() { Close(); }

void TConnection::Close() { mysql_close(&_mysql); }

void TConnection::Open() {
    MYSQL* mysql_ptr = mysql_init(&_mysql);
    if (!mysql_ptr) throw std::runtime_error("Errore mysql_init");

    my_bool reconnect;
    if (mysql_get_option(&_mysql, MYSQL_OPT_RECONNECT, &reconnect)) {
        throw std::runtime_error("mysql_get_option failed");
    }

    MYSQL* my = mysql_real_connect(&_mysql, fHost.c_str(), fUser.c_str(),
                                   fPassword.c_str(), fDb.c_str(), 0, nullptr, 0);
    if (!my) throw std::runtime_error(mysql_error(&_mysql));
    fConnected = true;
}

// ========================= TQuery =========================
TQuery::TQuery(TConnection& connection) : fConnection(&connection) {
    mysql = &fConnection->_mysql;
    isConnectDirect = false;
}

TQuery::TQuery(TConnection* connection) : fConnection(connection) {
    mysql = &fConnection->_mysql;
    isConnectDirect = false;
}

TQuery::TQuery(std::string_view host, std::string_view user,
               std::string_view password, std::string_view db) :
    fConnection(new TConnection(host, user, password, db)), isConnectDirect(true)
{
    mysql = &fConnection->_mysql;
}

TQuery::~TQuery() {
    if (isConnectDirect && fConnection) delete fConnection;
    Close();
}

TQuery& TQuery::operator=(TConnection& connection) {
    if (connection.Active) {
        if (fConnection && fConnection->Active) fConnection->Close();
        fConnection = &connection;
        mysql = &fConnection->_mysql;
        isConnectDirect = false;
    }
    return *this;
}

// ========================= Retry =========================
static inline bool should_retry_mysql(unsigned int err) {
    return err == 1213 || err == 1205; // deadlock / lock wait timeout
}

bool TQuery::run_query_with_retry(std::string_view query,
                                  int max_retries,
                                  unsigned int& last_errno,
                                  std::string& last_error) {
    using namespace std::chrono;
    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int> jitter(0, 100);

    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        mysql_rollback(mysql);
        int rc = mysql_query(mysql, query.data());
        if (rc == 0) return true;

        last_errno = mysql_errno(mysql);
        last_error = mysql_error(mysql);

        if (should_retry_mysql(last_errno) && attempt < max_retries) {
            int wait_ms = 150 * (1 << (attempt - 1)) + jitter(rng);
            std::this_thread::sleep_for(milliseconds(wait_ms));
            continue;
        }
        return false;
    }
    return false;
}

// ========================= Open =========================
void TQuery::Open(std::string_view query) {
    SQL.clear();
    SQL.push_back(std::string(query));
    Open();
}

void TQuery::Open() {
    std::string query;
    for (auto& s : SQL) query += s + " ";

    unsigned int last_errno = 0;
    std::string last_error;
    if (!run_query_with_retry(query, 5, last_errno, last_error)) {
        std::stringstream ss;
        ss << "Errore mysql_query: " << query
           << " (errno=" << last_errno << ") " << last_error;
        throw std::runtime_error(ss.str());
    }

    Close();
    _res = mysql_store_result(mysql);
    if (_res) {
        _row = mysql_fetch_row(_res);
        current_record = 0;
        fRecord = mysql_num_rows(_res);
        fIsOpen = true;
    } else if (mysql_field_count(mysql) == 0) {
        fRecord = mysql_affected_rows(mysql);
    } else {
        throw std::runtime_error(mysql_error(mysql));
    }
    SQL.clear();
}

// ========================= FieldByName =========================
std::string TQuery::FieldByName(std::string_view nome_campo) const {
    std::string name(nome_campo);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c){ return std::toupper(c); });

    if (!_res) throw std::runtime_error(mysql_error(mysql));

    int n_field = mysql_num_fields(_res);
    for (int i = 0; i < n_field; ++i) {
        MYSQL_FIELD* f = mysql_fetch_field_direct(_res, i);
        std::string s = f->name;
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){ return std::toupper(c); });
        if (s == name) return _row && _row[i] ? _row[i] : "";
    }
    throw std::runtime_error("Campo " + std::string(nome_campo) + " non trovato");
}

// ========================= Cursor =========================
void TQuery::Close() {
    if (_res) mysql_free_result(_res);
    _res = nullptr;
    fIsOpen = false;
}

void TQuery::First() { mysql_data_seek(_res, 0); _row = mysql_fetch_row(_res); current_record = 0; }
void TQuery::Next() { ++current_record; mysql_data_seek(_res, current_record); _row = mysql_fetch_row(_res); }
void TQuery::Prior() { --current_record; mysql_data_seek(_res, current_record); _row = mysql_fetch_row(_res); }
void TQuery::MoveBy(int n) { current_record = n < fRecord ? n : 0; mysql_data_seek(_res, current_record); _row = mysql_fetch_row(_res); }

// ========================= Add =========================
void TQuery::Add(std::string_view str) { SQL.push_back(std::string(str)); }

// ========================= delete_tipoP_in_batches =========================
bool delete_tipoP_in_batches(TQuery& q, size_t batch_size, int max_retries) {
    (void)max_retries;
    while (true) {
        std::stringstream ss;
        ss << "DELETE FROM index_stars_ WHERE tipo='P' LIMIT " << batch_size;
        q.Open(ss.str());
        if (q.fRecord == 0) break;
    }
    return true;
}
