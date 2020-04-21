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

///------------------------------------------------------------------------------------------------------------------///

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
};

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

///------------------------------------------------------------------------------------------------------------------///

///THREAD_SAFE QUEUE------------------------------------------------------------------------------------------

template<class T>
class safeQueue {
    std::deque<T> que;
    mutable std::mutex m;
    std::condition_variable cvNotEmpty;
    std::condition_variable cvNotMax;
    size_t max_size = 10;

public:
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

int main(){
    std::cout << read_archive("/home/vlad/Desktop/2_year/ACS/lab_4/pro_1/arch_3.zip") << std::endl;
}