///------------------------------------------------------------------------------------------------------------------///
/// INCLUDES

#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>
#include <map>
#include <archive.h>
#include <archive_entry.h>
#include <string>
#include <tuple>
#include <vector>
#include <filesystem>


#include <boost/locale.hpp>
#include <boost/locale/boundary.hpp>
#include <boost/locale/conversion.hpp>
#include <fstream>
#include <thread>
#include <sys/stat.h>
#include <condition_variable>
#include <deque>

///THREAD_SAFE QUEUE------------------------------------------------------------------------------------------

template<class T>
class safeQueue {
    std::deque<T> que;
    mutable std::mutex m;
    std::condition_variable cvNotEmpty;
    std::condition_variable cvNotMax;

public:
    size_t max_size = 10;

    safeQueue(){}

    void setMaxSize(size_t newMax) {
        std::lock_guard<std::mutex> lg{m};
        max_size = newMax;
    }

    auto size() const {
        std::lock_guard<std::mutex> lg{m};
        return que.size();
    }



    void pushFront(T item) {
        std::unique_lock<std::mutex> lg{m};
        cvNotMax.wait(lg, [this](){return max_size != que.size();});

        que.push_front(item);
        cvNotEmpty.notify_one();
    }

    void pushBack(T item) {
        std::unique_lock<std::mutex> lg{m};
        cvNotMax.wait(lg, [this](){return max_size != que.size();});

        que.push_back(item);
        cvNotEmpty.notify_one();
    }



    T popBack(){
        std::unique_lock<std::mutex> lg{m};
        cvNotEmpty.wait(lg, [this](){return que.size() != 0;});
        T result = que.back();
        que.pop_back();
        return result;
    }

    T popFront(){
        std::unique_lock<std::mutex> lg{m};
        cvNotEmpty.wait(lg, [this](){return que.size() != 0;});
        T result = que.front();
        que.pop_front();
        return result;
    }
};

// -----------------------------------------------------------------------------------------------------------------

std::string define_ext( std::string filename ){
    char extensions[][8] = {"txt", "zip", "tar", "sbx", "iso"};

    for( auto &x: extensions ){
        if ( filename.substr(filename.find_last_of(".") + 1) == x ){
            return  x;
        }
    }
    return std::string("");
}

bool check_file( std::string filename ){

    auto result = false;
    char extensions[][8] = {"txt", "zip", "tar", "sbx", "iso"};

    for( auto &x: extensions ){
        result |= filename.substr(filename.find_last_of(".") + 1) == x;
    }
    return result;
}

int directory_size( std::string directory ){
    int amount = 0;

    using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

    for (const auto& dirEntry : recursive_directory_iterator( directory )){
        amount += check_file( dirEntry.path().string() );
    }
    return amount;
}

auto get_file_by_index( std::string directory, int index ){
    using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

    int count = -1;
    std::string result = "";

    for (const auto& dirEntry : recursive_directory_iterator( directory )){

        result = dirEntry.path().string();
        count += check_file(result);

        if (count == index){
            return result;
        }
    }
    result = "";
    return result;
}

// Archive manipulations
///------------------------------------------------------------------------------------------------------------------///

auto read_as_raw( const char* filename ){
    std::ifstream raw_file(filename, std::ios::binary);
    std::ostringstream buffer_ss;
    buffer_ss << raw_file.rdbuf();
    std::string buffer{buffer_ss.str()};
    return buffer;
}

auto read_archive_from_memory( std::string buffer ){
    struct archive *a;
    struct archive_entry *entry;

    a = archive_read_new();
    archive_read_support_filter_all(a);
    std::string content, total;

    archive_read_support_format_all(a);
    archive_read_open_memory(a, buffer.c_str(), 10240);

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {

        if ( check_file( archive_entry_pathname(entry) ) && archive_entry_size(entry) < 10000000 ){
            auto size = archive_entry_size(entry);
            content = std::string(size, 0);

            archive_read_data(a, &content[0], content.size());
            total += content + "\n";
        }
        archive_read_data_skip(a);
    }
    archive_read_free(a);
    return total;
}

///------------------------------------------------------------------------------------------------------------------///

///------------------------------------------------------------------------------------------------------------------///
auto archive_size(std::string archivename){
    struct archive *a;
    struct archive_entry *entry;

    int r;
    int size = 0;

    a = archive_read_new();
    archive_read_support_filter_all(a);

    archive_read_support_format_all(a);
    r = archive_read_open_filename(a, archivename.c_str(), 10240);

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        size = check_file( archive_entry_pathname(entry) ) && archive_entry_size(entry) < 10000000 ? size + 1: size;
        archive_read_data_skip(a);
    }
    r = archive_read_free(a);
    return size;
}
///------------------------------------------------------------------------------------------------------------------///
auto archive_file_size_by_index( std::string archivename, int index ){
    int current = -1;
    struct archive *a;

    struct archive_entry *entry;
    int r;

    a = archive_read_new();
    archive_read_support_filter_all(a);

    archive_read_support_format_all(a);
    r = archive_read_open_filename(a, archivename.c_str(), 10240);

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        current = check_file( archive_entry_pathname(entry) ) && archive_entry_size(entry) < 10000000 ? current + 1: current;

        if(current == index){
            current = archive_entry_size(entry);
            r = archive_read_free(a);
            return current;
        }

        archive_read_data_skip(a);
    }
    r = archive_read_free(a);
    return -1;
}

auto archive_file_content_by_index( std::string archivename, int index ){
    std::string content;
    int current = -1;
    struct archive *a;

    struct archive_entry *entry;
    int r;

    a = archive_read_new();
    archive_read_support_filter_all(a);

    archive_read_support_format_all(a);
    r = archive_read_open_filename(a, archivename.c_str(), 10240);

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        current = check_file( archive_entry_pathname(entry) ) && archive_entry_size(entry) < 10000000 ? current + 1: current;

        if(current == index){
            auto size = archive_entry_size(entry);
            content = std::string(size, 0);

            archive_read_data(a, &content[0], content.size());
            r = archive_read_free(a);
            return content;
        }

        archive_read_data_skip(a);
    }
    r = archive_read_free(a);
    content = "";
    return content;
}

void fill_queue( std::string directory, safeQueue<std::string> &sq ){

    using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

    for (const auto& dirEntry : recursive_directory_iterator( directory )){

        if ( check_file( dirEntry.path().string() ) ){
//            sq.pushBack( { read_as_raw(dirEntry.path().string().c_str()), define_ext( dirEntry.path().string() ) } ) );
            std::cout << define_ext( dirEntry.path().string() ) << "\n";
            }
        }
    }

///---------------------------------------------CONSECUTIVE IMPLEMENTATION---------------------------------------------------------------///

auto consecutive( std::string directoryname, size_t que_max_size ){

    auto size = directory_size(directoryname);
    safeQueue<std::string> sq;
    sq.setMaxSize(que_max_size);

    for( int i = 0; i < 5; i++ ){

        auto current_archive = get_file_by_index(directoryname, i);
        auto ar_size = archive_size(current_archive);

        for ( int i = 0; i < ar_size; i++ ){
            sq.pushBack( archive_file_content_by_index(current_archive, i ) );

            if ( sq.size() == sq.max_size - 1 ){
//                make the conclusions
            }
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------

int main(){
//     for (int i = 0; i <= 92019; i++){
//          std::cout << i << "\n";
//            std::cout << get_file_by_index("../guttenberg_2020_03_06", i) << "         " << i << "\n";
//     }
//     std::cout << get_file_by_index("../guttenberg_2020_03_06", 1) << "\n";
//    // std::cout << archive_size("guttenberg_2020_03_06/1/16/16-0.zip") << "\n";
//     std::cout << archive_file_content_by_index("guttenberg_2020_03_06/1/16/16-0.zip",0) << "\n";
//    // directory_size("guttenberg_2020_03_06");
//    // Compilation Command: g++ -std=c++17 -O3 main_3.cpp -o main -larchive ;

//    consecutive("../guttenberg_2020_03_06");
//    fill_queue("../guttenberg_2020_03_06", sq);
    return 0;
}