#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXNetSystem.h>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <thread>
#include <vector>
#include <map>
#include <ctime>
#include <string>

// Not: Gerçek işlem için Binance imzalama fonksiyonları gereklidir. 
// Bu örnek yapı, senin terminal botunun gerçek işlem mantığına dökülmüş halidir.

using json = nlohmann::json;

// --- RENK VE STİL KODLARI ---
#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

enum BotState { IDLE, BOUGHT };

// --- GERÇEK İŞLEM AYARLARI ---
const std::string API_KEY = "YOUR_BINANCE_API_KEY";
const std::string SECRET_KEY = "YOUR_BINANCE_SECRET_KEY";

const double BUY_THRESHOLD = -0.25;    // %0.25 düşüşte satın al
const double TRAILING_TRIGGER = 0.45;  // %0.45 yükseldiğinde kâr takibini başlat
const double TRAILING_DROP = 0.05;     // Zirveden %0.05 saptığında sat
const double STOP_LOSS = -0.30;        // %0.30 düşerse zararı kes

struct TradingBot {
    std::string symbol;
    BotState state = IDLE;
    double entryPrice = 0.0;
    double initialPrice = 0.0;
    double currentPrice = 0.0;
    double highestPrice = 0.0; 
    bool isTrailing = false;   
    bool priceInitialized = false;
    double actualBalance = 0.0; // Gerçek hesaptaki coin miktarı
};

std::map<std::string, TradingBot> bots;

// --- BINANCE EMİR GÖNDERME FONKSİYONLARI (TASLAK) ---
void executeMarketOrder(std::string symbol, std::string side, double amount) {
    // Burada Binance API'sine POST isteği atılır.
    // Side: BUY veya SELL
    std::cout << BOLD << RED << "\n[EMİR TETİKLENDİ] " << side << " -> " << symbol << RESET << std::endl;
}

void logTrade(std::string symbol, std::string type, double price, double profit) {
    std::ofstream file("real_trade_log.csv", std::ios::app);
    if (file.is_open()) {
        time_t now = time(0);
        char* dt = ctime(&now);
        std::string timeStr(dt);
        if (!timeStr.empty()) timeStr.pop_back();

        file << timeStr << "," << symbol << "," << type << "," << price << "," 
             << std::fixed << std::setprecision(2) << profit << "%\n";
        file.close();
    }
}

// --- GERÇEK STRATEJİ MEKANİZMASI ---
void runStrategy(TradingBot& bot) {
    if (!bot.priceInitialized) return;

    double changeFromStart = ((bot.currentPrice - bot.initialPrice) / bot.initialPrice) * 100;

    // 1. GERÇEK SATIN ALMA
    if (bot.state == IDLE && changeFromStart <= BUY_THRESHOLD) {
        executeMarketOrder(bot.symbol, "BUY", 15.0); // Örn: 15 USDT'lik alım
        
        bot.state = BOUGHT;
        bot.entryPrice = bot.currentPrice;
        bot.highestPrice = bot.currentPrice;
        bot.isTrailing = false;

        logTrade(bot.symbol, "REAL_BUY", bot.entryPrice, 0);
    } 
    // 2. GERÇEK SATIŞ (Trailing & Stop Loss)
    else if (bot.state == BOUGHT) {
        double profit = ((bot.currentPrice - bot.entryPrice) / bot.entryPrice) * 100;

        if (bot.currentPrice > bot.highestPrice) bot.highestPrice = bot.currentPrice;
        if (!bot.isTrailing && profit >= TRAILING_TRIGGER) bot.isTrailing = true;

        bool shouldSell = false;
        std::string reason = "";

        if (bot.isTrailing) {
            double dropFromPeak = ((bot.highestPrice - bot.currentPrice) / bot.highestPrice) * 100;
            if (dropFromPeak >= TRAILING_DROP) { shouldSell = true; reason = "TRAILING_SELL"; }
        } else if (profit <= STOP_LOSS) {
            shouldSell = true; reason = "STOP_LOSS_SELL";
        }

        if (shouldSell) {
            executeMarketOrder(bot.symbol, "SELL", 0.0); // Elindeki tüm miktarı sat
            bot.state = IDLE;
            logTrade(bot.symbol, reason, bot.currentPrice, profit);
        }
    }
}

int main() {
    ix::initNetSystem();
    ix::WebSocket webSocket;

    // Takip edilecek gerçek pariteler
    std::vector<std::string> symbols = {"btcusdt", "ethusdt", "solusdt", "bnbusdt", "xrpusdt"};
    std::string streams = "";
    for (size_t i = 0; i < symbols.size(); ++i) {
        streams += symbols[i] + "@ticker" + (i == symbols.size() - 1 ? "" : "/");
    }

    webSocket.setUrl("wss://stream.binance.com:9443/stream?streams=" + streams);
    
    webSocket.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            try {
                auto j = json::parse(msg->str);
                auto data = j["data"];
                std::string s = data["s"];
                double p = std::stod(data["c"].get<std::string>());
                
                auto& bot = bots[s];
                bot.currentPrice = p;
                if (!bot.priceInitialized) { 
                    bot.initialPrice = p; 
                    bot.symbol = s; 
                    bot.priceInitialized = true; 
                }
                runStrategy(bot);
            } catch (...) {}
        }
    });

    webSocket.start();

    // TERMİNAL ARAYÜZÜ
    while (true) {
        system("cls"); // Windows için ekranı temizle
        
        std::cout << BOLD << YELLOW << ">>> BINANCE REAL-TIME TERMINAL v3.0 <<<" << RESET << std::endl;
        std::cout << "Mod: " << RED << "LIVE TRADING (GERCEK HESAP)" << RESET << "\n" << std::endl;
        
        std::cout << CYAN << "+----------+------------+---------+---------+----------+" << RESET << std::endl;
        std::cout << CYAN << "| SEMBOL   | FIYAT      | FARK %  | HEDEFE  | DURUM    |" << RESET << std::endl;
        std::cout << CYAN << "+----------+------------+---------+---------+----------+" << RESET << std::endl;

        for (auto const& [name, bot] : bots) {
            if (bot.priceInitialized) {
                double diff = ((bot.currentPrice - bot.initialPrice) / bot.initialPrice) * 100;
                double dist = BUY_THRESHOLD - diff;

                std::cout << CYAN << "| " << RESET << std::left << std::setw(8) << bot.symbol 
                          << CYAN << " | " << RESET << std::setw(10) << bot.currentPrice
                          << CYAN << " | " << RESET << (diff >= 0 ? GREEN : RED) << std::showpos << std::setprecision(2) << std::setw(7) << diff << "%" << RESET
                          << CYAN << " | " << RESET << (dist <= 0 ? GREEN : YELLOW) << std::setprecision(2) << std::setw(7) << dist << "%" << RESET
                          << CYAN << " | " << RESET << (bot.state == BOUGHT ? (bot.isTrailing ? GREEN : YELLOW) : RESET) 
                          << (bot.state == BOUGHT ? (bot.isTrailing ? "TAKIP" : "ELDE") : "BEKLE") << RESET << std::endl;
            }
        }
        std::cout << CYAN << "+----------+------------+---------+---------+----------+" << RESET << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    ix::uninitNetSystem();
    return 0;
}