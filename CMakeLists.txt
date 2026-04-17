cmake_minimum_required(VERSION 3.10)

# vcpkg yolunu en üste çekiyoruz
set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(CryptoBot)

set(CMAKE_CXX_STANDARD 17)

# Gerekli Paketleri Bul
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ixwebsocket CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(CryptoBot main.cpp)

# Windows sistem kütüphaneleri (SSL ve Network için şart)
if(WIN32)
    set(WINDOWS_LIBS Bcrypt.lib Ws2_32.lib Crypt32.lib)
endif()

# Tüm kütüphaneleri projeye bağla
target_link_libraries(CryptoBot PRIVATE 
    CURL::libcurl 
    nlohmann_json::nlohmann_json
    ixwebsocket::ixwebsocket
    OpenSSL::SSL
    OpenSSL::Crypto
    ${WINDOWS_LIBS}
)