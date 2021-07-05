/**
 * Citations
 *
 *  File writing
 *      https://stackoverflow.com/questions/15667530/fstream-wont-create-a-file
 *
 *  fstream constructor flags
 *      http://www.cplusplus.com/reference/fstream/ofstream/open/
 */

#pragma once
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

class FileManager {
   public:
    FileManager(const std::string& filename, bool append = false);
    ~FileManager();

    void write(const std::string& str);
    void flush();

   private:
    const std::string _FILENAME;
    std::fstream _FILESTREAM;
};

FileManager::FileManager(const std::string& filename, bool append)
    : _FILENAME(filename) {
    _FILESTREAM.open(filename,
                     std::fstream::out |
                         ((append) ? std::fstream::app : std::fstream::trunc));
    if (_FILESTREAM.fail()) {
        throw std::runtime_error("\'" + filename + "\' could not be opened.");
    }
}

FileManager::~FileManager() { _FILESTREAM.close(); }

void FileManager::write(const std::string& str) { _FILESTREAM << str; }
void FileManager::flush() { _FILESTREAM.flush(); }