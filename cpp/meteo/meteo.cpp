#include <iostream>
#include <string>
#include <ctime>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

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

    // Open-Meteo API: prendo temperatura attuale e umidità oraria
    // current_weather fornisce temperatura corrente, ma non umidità
    // umidità la prendiamo da hourly=relativehumidity_2m
    std::string url = "https://api.open-meteo.com/v1/forecast?latitude=" + std::to_string(latitude)
                    + "&longitude=" + std::to_string(longitude)
                    + "&current_weather=true"
                    + "&hourly=relativehumidity_2m";

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "Errore nella richiesta: " << curl_easy_strerror(res) << "\n";
        } else {
            try {
                auto j = json::parse(readBuffer);

                if(j.contains("current_weather")) {
                    auto weather = j["current_weather"];
                    std::cout << "Temperatura: " << weather["temperature"] << " °C\n";

                    // Ora UTC attuale (formato ISO 8601)
                    time_t now = time(nullptr);
                    struct tm *utc_tm = gmtime(&now);

                    char time_str[20];
                    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:00", utc_tm);
                    std::string current_hour(time_str);

                    // estrai umidità dalla sezione hourly se presente
                    if(j.contains("hourly") && j["hourly"].contains("time") && j["hourly"].contains("relativehumidity_2m")) {
                        auto times = j["hourly"]["time"];
                        auto humidities = j["hourly"]["relativehumidity_2m"];

                        // cerca l’indice del tempo corrente
                        int idx = -1;
                        for(size_t i = 0; i < times.size(); i++) {
                            std::string t = times[i];
                            if(t.find(current_hour) == 0) { // match con ora inizio
                                idx = i;
                                break;
                            }
                        }

                        if(idx != -1) {
                            std::cout << "Umidità relativa: " << humidities[idx] << " %\n";
                        } else {
                            std::cout << "Umidità relativa: dati non disponibili per l’ora attuale\n";
                        }
                    } else {
                        std::cout << "Umidità relativa: dati non disponibili\n";
                    }
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
