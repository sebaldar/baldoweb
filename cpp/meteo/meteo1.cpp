#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"  // https://github.com/nlohmann/json

using json = nlohmann::json;

// funzione di callback per libcurl (scrive la risposta in una stringa)
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main() {
    double latitude, longitude, elevation;
    std::cout << "Inserisci latitudine: ";
    std::cin >> latitude;
    std::cout << "Inserisci longitudine: ";
    std::cin >> longitude;
    std::cout << "Inserisci altezza (metri): ";
    std::cin >> elevation;

    // costruisco la URL per chiamare l'API Open-Meteo, prendiamo meteo corrente (temperature, vento, ecc)
    std::string url = "https://api.open-meteo.com/v1/forecast?latitude=" + std::to_string(latitude)
                    + "&longitude=" + std::to_string(longitude)
                    + "&current_weather=true";

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // usa la callback per salvare la risposta
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // eseguo la richiesta
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "Errore nella richiesta: " << curl_easy_strerror(res) << "\n";
        } else {
            try {
                auto j = json::parse(readBuffer);

                if(j.contains("current_weather")) {
                    auto weather = j["current_weather"];
                    std::cout << "Temperatura: " << weather["temperature"] << " °C\n";
                    std::cout << "Velocità vento: " << weather["windspeed"] << " km/h\n";
                    std::cout << "Direzione vento: " << weather["winddirection"] << "°\n";
                    std::cout << "Condizioni: " << (weather.contains("weathercode") ? std::to_string(weather["weathercode"].get<int>()) : "N/A") << "\n";
                } else {
                    std::cout << "Dati meteo non disponibili\n";
                }
            } catch(std::exception& e) {
                std::cerr << "Errore nel parsing JSON: " << e.what() << "\n";
            }
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    return 0;
}
