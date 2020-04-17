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
auto check_file( std::string filename ){
    if (filename.substr(filename.find_last_of(".") + 1) == "txt"){
        return 1;
    }
    else if (filename.substr(filename.find_last_of(".") + 1) == "zip"){
        return 2;
    }
    return 0;
}



std::string read_archive( const char *filename ){

    struct archive *a;
    struct archive_entry *entry;

    a = archive_read_new();
    archive_read_support_filter_all(a);
    std::string total = "";

    archive_read_support_format_all(a);
    archive_read_open_filename(a, filename, 10240);

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {

        if ( check_file( archive_entry_pathname(entry)) == 2 ){
            total += read_archive( archive_entry_pathname(entry) );
        }

        else if ( check_file(archive_entry_pathname(entry)) &&  archive_entry_size(entry) < 10000000  ){
            auto size = archive_entry_size(entry);
            auto content = std::string(size, 0);

            archive_read_data( a, &content[0], content.size() );
            total += content + "\n";
        }
        archive_read_data_skip(a);
    }
    archive_read_free(a);

    return total;
}

///THREAD_SAFE QUEUE------------------------------------------------------------------------------------------

template<class T>
class safeQueue {
    std::deque<int> que;
    mutable std::mutex m;
    std::condition_variable cv;

public:
    safeQueue(){}

    size_t size() const {
        std::lock_guard<std::mutex> lg{m};
        return que.size();
    }

    void push(T item) {
        std::unique_lock<std::mutex> lg{m};
        que.push_front(item);
        cv.notify_one();
    }

    T pop(){
        std::unique_lock<std::mutex> lg{m};
        cv.wait(lg, [this](){ return que.size() == 0; });
        T d = que.back();
        que.pop_back();
        return d;
    }
};
// -----------------------------------------------------------------------------------------------------------------

int main(){
    std::cout << read_archive("/home/vlad/Desktop/2_year/ACS/lab_4/pro_1/arch_3.zip") << std::endl;
}