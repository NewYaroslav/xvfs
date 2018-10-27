#include "xvfs.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>


static long long xvfs_crc64_table[256];
static bool is_svfs_crc64_table = false;

bool xvfs::check_file(std::string file_name) {
   std::ifstream file;
   file.open(file_name);
   if(!file) return false;
   file.close();
   return true;
}

bool xvfs::create_file(std::string file_name) {
    std::fstream file(file_name, std::ios::out | std::ios::app);
    if(!file) return false;
    file.close();
    return true;
}

xvfs::xvfs(std::string file_name) {
    xvfs::file_name = file_name; // запоминаем имя файла виртуальноц файловой системы
    generate_table(); // инициализируем crc таблицу
    if(!check_file(file_name)) {
        is_open_file = create_file(file_name);
        if(is_open_file) {
            init_xvfs_header();
            save_header();
        }
    } else {
        is_open_file = read_header();
    }
}

xvfs::xvfs(std::string file_name, int sector_size) {
    const unsigned long min_sector_size = 32;
    xvfs::file_name = file_name; // запоминаем имя файла виртуальноц файловой системы
    generate_table(); // инициализируем crc таблицу
    if(!check_file(file_name)) {
        if(sector_size < min_sector_size) {
            is_open_file = false;
            return;
        }
        is_open_file = create_file(file_name);
        if(is_open_file) {
            init_xvfs_header(sector_size);
            save_header();
        }
    } else {
        is_open_file = read_header();
    }
}

bool xvfs::read_header() {
    //std::cout << "read_header" << std::endl;
    std::ifstream fin(file_name, std::ios_base::binary); // открываем файл
    if (!fin.is_open()) {
        return false; // не смогли октрыть файл
    }
    fin.seekg(0, std::ios::end);
    unsigned long file_size = fin.tellg();
    fin.seekg(0, std::ios::beg);
    fin.clear();

    unsigned long header_size = 0;
    // читаем размер
    fin.read(reinterpret_cast<char *>(&header_size),sizeof (header_size));
    //std::cout << "header_size " << header_size << std::endl;
    if(header_size > file_size) {
        fin.close();
        //std::cout << "error header_size > file_size" << std::endl;
        return false;
    }
    // читаем размер сектора
    fin.read(reinterpret_cast<char *>(& xvfs_header.sector_size),sizeof ( xvfs_header.sector_size));
    //std::cout << "sector_size " << xvfs_header.sector_size << std::endl;

    const unsigned long min_sector_size = 32;
    if(xvfs_header.sector_size < min_sector_size) {
        fin.close();
        //std::cout << "error sector_size < min_sector_size" << std::endl;
        return false;
    }
    // далее уже нельзя просто считывать заголовок, читаем по секторам
    char* buf = new char[xvfs_header.sector_size];
    char* header_data = new char[header_size];
    unsigned long len = 0;
    unsigned long next_sector = 0;
    unsigned long next_pos = 0;

    while(next_sector != 0xFFFFFFFF) {
        //std::cout << "next_pos " << next_pos << std::endl;
        fin.seekg(next_pos, std::ios::beg);
        fin.read(buf, xvfs_header.sector_size);
        if(!fin) {
            fin.close();
            delete[] buf;
            delete[] header_data;
            //std::cout << "error !fin" << std::endl;
            return false;
        }
        unsigned long old_len = len; // запоминаем длину
        len += xvfs_header.sector_size - sizeof(unsigned long); // находим количество считанных байтов
        if(len >= header_size) len = header_size;
        std::memcpy(header_data + old_len, buf, len - old_len);
        if(len == header_size) break; // выходим, если все считали
        // получаем следующий сектор для считывания
        next_sector = ((unsigned long*)(buf + (xvfs_header.sector_size - sizeof(unsigned long))))[0];
        // получаем следующую позицию для смещения
        next_pos = next_sector * xvfs_header.sector_size;
        //std::cout << "next_sector " << next_sector << std::endl;
    }
    fin.close();
    if(len != header_size) {
        delete[] buf;
        delete[] header_data;
        //std::cout << "error len != header_size" << std::endl;
        return false;
    }
    // парсим header_data
    unsigned long files_size = 0;
    unsigned long offset = sizeof (header_size) + sizeof (xvfs_header.sector_size);
    files_size = ((unsigned long*)(header_data + offset))[0];
    //std::cout << "files_size " << files_size << std::endl;
    offset += sizeof (files_size);
    xvfs_header.files.resize(files_size);
    for(unsigned long i = 0; i < files_size; ++i) {
        xvfs_header.files[i] = ((_xvfs_file_header*)(header_data + offset))[0];
        offset += sizeof (_xvfs_file_header);
        //std::cout << "xvfs_header.files[" << i << "].hash " << xvfs_header.files[i].hash << std::endl;
    }
    unsigned long empty_sectors_size = 0;
    empty_sectors_size = ((unsigned long*)(header_data + offset))[0];
    //std::cout << "empty_sectors_size " << empty_sectors_size << std::endl;
    offset += sizeof (empty_sectors_size);
    xvfs_header.empty_sectors.resize(empty_sectors_size);
    for(unsigned long i = 0; i < empty_sectors_size; ++i) {
        xvfs_header.empty_sectors[i] = ((unsigned long*)(header_data + offset))[0];
        offset += sizeof (unsigned long);
        //std::cout << "xvfs_header.empty_sectors[" << i << "] " << xvfs_header.empty_sectors[i] << std::endl;
    }
    delete[] buf;
    delete[] header_data;
    return true;
}

long xvfs::read_data(unsigned long start_sector, char* file_data, unsigned long file_size) {
    //std::cout << "read_data" << std::endl;
    if(!is_open_file) return -1;
    std::ifstream fin(file_name, std::ios_base::binary); // открываем файл
    if (!fin.is_open()) {
        return -1; // не смогли октрыть файл
    }
    unsigned long next_sector = start_sector;
    unsigned long next_pos = start_sector * xvfs_header.sector_size; // получаем начальный сектор
    unsigned long len = 0;
    char* buf = new char[xvfs_header.sector_size];
    while(next_sector != 0xFFFFFFFF) {
        fin.seekg(next_pos, std::ios::beg);
        if(!fin) {
            fin.close();
            delete[] buf;
            return -1;
        }
        fin.read(buf, xvfs_header.sector_size);
        if(!fin) {
            fin.close();
            delete[] buf;
            return -1;
        }
        unsigned long old_len = len;
        len += xvfs_header.sector_size - sizeof(unsigned long);
        if(len > file_size) len = file_size;
        std::memcpy(file_data + old_len, buf, len - old_len);
        if(len == file_size) break;
        next_sector = ((unsigned long*)(buf + (xvfs_header.sector_size - sizeof(unsigned long))))[0];
        next_pos = next_sector * xvfs_header.sector_size;
    }
    fin.close();
    delete[] buf;
    return len;
}

long xvfs::write_data(unsigned long start_sector, char* file_data, unsigned long file_size) {
    if(!is_open_file) return -1;
    std::fstream fout(file_name, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
    if (!fout.is_open()) {
        return -1; // не смогли октрыть файл
    }
    unsigned long next_sector = start_sector;
    unsigned long next_pos = start_sector * xvfs_header.sector_size; // получаем начальный сектор
    unsigned long len = 0;

    //std::cout << "next_pos " << next_pos << std::endl;

    fout.seekg (0, std::ios::end);
    unsigned long real_file_size = fout.tellg();
    fout.seekg (next_pos, std::ios::beg);
    fout.clear();
    //std::cout << "real_file_size " << real_file_size << std::endl;

    char* buf = new char[xvfs_header.sector_size];
    std::memset(buf, 0, xvfs_header.sector_size);

    bool is_end_file = false;

    while(1) {
        //std::cout << "write sector " << next_sector << std::endl;
        unsigned long old_len = len;
        len += xvfs_header.sector_size - sizeof(unsigned long);
        if(len >= file_size) {
            len = file_size;
            next_sector = 0xFFFFFFFF;
        } else if(!is_end_file) {
            // узнаем номер следующего сектора
            unsigned long offset_sector = next_pos + xvfs_header.sector_size - sizeof(unsigned long); // смещение в файле, где лежит сектор
            if(offset_sector < real_file_size) { // если смещение меньше размера файла
                fout.seekg(offset_sector, std::ios::beg);
                fout.clear();
                fout.read(reinterpret_cast<char *>(&next_sector),sizeof(unsigned long));
                fout.seekg(next_pos, std::ios::beg);
                //std::cout << "read next_sector " << next_sector << std::endl;
                if(next_sector == 0xFFFFFFFF) {
                    // выбираем один из пустых секторов
                    // или перемещаемся в конец файла
                    if(xvfs_header.empty_sectors.size() > 0) {
                        next_sector = xvfs_header.empty_sectors[0];
                        xvfs_header.empty_sectors.erase(xvfs_header.empty_sectors.begin());
                        //std::cout << "  empty_sectors - " << next_sector << std::endl;
                    } else {
                        fout.seekg(0, std::ios::end);
                        unsigned long _size = fout.tellg();
                        next_sector = _size / xvfs_header.sector_size;
                        //std::cout << "  sectors + " << next_sector << std::endl;
                    }
                }
            } else {
                is_end_file = true;
                next_sector = (real_file_size / xvfs_header.sector_size) + 1;
                //std::cout << "  is_end_file = true" << std::endl;
            }
        } else {
            next_sector++;
        }

        //std::cout << "next_sector " << next_sector << std::endl;
        std::memcpy(buf, file_data + old_len, len - old_len);
        std::memcpy(buf + xvfs_header.sector_size - sizeof(unsigned long), &next_sector, sizeof(unsigned long));

        fout.seekg (next_pos, std::ios::beg);
        fout.write(buf, xvfs_header.sector_size);
        //real_file_size += xvfs_header.sector_size;

        if(!fout) {
            fout.close();
            delete[] buf;
            return -1;
        }
        if(len == file_size) break;
        next_pos = next_sector * xvfs_header.sector_size;
    }
    fout.close();
    delete[] buf;
    return len;
}

bool xvfs::clear_data(unsigned long start_sector, bool is_first_sector) {
    if(!is_open_file) return false;
    std::fstream fout(file_name, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
    if (!fout.is_open()) {
        return false; // не смогли октрыть файл
    }
    unsigned long next_sector = start_sector;
    unsigned long next_pos = start_sector * xvfs_header.sector_size; // получаем начальный сектор
    while(1) {
        // узнаем номер следующего сектора
        unsigned long old_next_sector = next_sector;
        fout.seekg(next_pos + xvfs_header.sector_size - sizeof(unsigned long), std::ios::beg);
        fout.read(reinterpret_cast<char *>(&next_sector),sizeof(unsigned long));

        if((!is_first_sector && old_next_sector != start_sector) || (is_first_sector)) {
            // добавляем сектора в массив пустых секторов
            // если файл перезаписывается, первый сектор не добавляется
            auto it = std::lower_bound(xvfs_header.empty_sectors.begin(), xvfs_header.empty_sectors.end(), old_next_sector);
            xvfs_header.empty_sectors.insert(it, old_next_sector);
            //std::cout << "clear sector " << old_next_sector << std::endl;
        }

        //std::cout << "clear sector " << old_next_sector << std::endl;
        if(next_sector == 0xFFFFFFFF) {
            break;
        }
        fout.seekg(next_pos + xvfs_header.sector_size - sizeof(unsigned long), std::ios::beg);
        unsigned long END_SECTOR = 0xFFFFFFFF;
        fout.clear();
        fout.write(reinterpret_cast<char *>(&END_SECTOR),sizeof(unsigned long));
        next_pos = next_sector * xvfs_header.sector_size;
    }
    fout.close();
    return true;
}

unsigned long xvfs::get_last_new_sector() {
    std::ifstream fin(file_name, std::ios_base::binary); // открываем файл
    fin.seekg(0, std::ios::end);
    unsigned long _size = fin.tellg();
    unsigned long last_sector = _size / xvfs_header.sector_size;
    fin.close();
    return last_sector;
}

bool xvfs::write_file(long long hash_vfs_file, char* data, unsigned long len) {
    if(!is_open_file) return false;
    long pos = binary_search_first(xvfs_header.files, hash_vfs_file, 0, xvfs_header.files.size() - 1);
    unsigned long start_sector;
    if(pos == -1) { // если файла нет
        if(xvfs_header.empty_sectors.size() > 0) { // если есть пустые сектора
            start_sector = xvfs_header.empty_sectors[0];
            xvfs_header.empty_sectors.erase(xvfs_header.empty_sectors.begin());
        } else { // если пустых секторов нет
            start_sector = get_last_new_sector();
        }
        // пишем файл
        if(write_data(start_sector, data, len) == -1) return false;
        // добавим файл в заголовок
        _xvfs_file_header i_file_header(hash_vfs_file, len, start_sector);

        if(xvfs_header.files.size() > 0) {
            auto it = std::lower_bound(xvfs_header.files.begin(), xvfs_header.files.end(), i_file_header);
            xvfs_header.files.insert(it, i_file_header);
        } else {
            xvfs_header.files.push_back(i_file_header);
        } //
    } else { // если файл уже существует
        start_sector = xvfs_header.files[pos].start_sector;
        if(std::ceil((double)len / (double)xvfs_header.sector_size) != std::ceil((double)xvfs_header.files[pos].size / (double)xvfs_header.sector_size)) { // если длина файла не совпадает
            // очистим данные
            if(!clear_data(start_sector, false)) return false;
            if(write_data(start_sector, data, len) == -1) return false;
        } else { // если длина файла совпадает
            // запишем данные туда же
            if(write_data(start_sector, data, len) == -1) return false;
        } //

        xvfs_header.files[pos] = _xvfs_file_header(hash_vfs_file, len, start_sector);
    } //

    // сохраняем заголовок
    return save_header();
}

bool xvfs::write_file(std::string vfs_file_name, char* data, unsigned long len) {
    if(!is_open_file) return false;
    long long hash_vfs_file = calculate_crc64(vfs_file_name);
    return write_file(hash_vfs_file, data, len);
}

long xvfs::get_len_file(long long hash_vfs_file) {
    if(!is_open_file) return -1;
    long pos = binary_search_first(xvfs_header.files, hash_vfs_file, 0, xvfs_header.files.size() - 1);
    if(pos == -1) {
        return -1;
    }
    return xvfs_header.files[pos].size;
}

long xvfs::get_len_file(std::string vfs_file_name) {
    if(!is_open_file) return -1;
    long long hash_vfs_file = calculate_crc64(vfs_file_name);
    return get_len_file(hash_vfs_file);
}

long xvfs::read_file(long long hash_vfs_file, char*& data) {
    if(!is_open_file) return -1;
    long pos = binary_search_first(xvfs_header.files, hash_vfs_file, 0, xvfs_header.files.size() - 1);
    if(pos == -1) {
        return -1;
    } else {
        bool is_biffer_init = false;
        if(data == NULL) {
            data = new char[xvfs_header.files[pos].size];
            is_biffer_init = true;
        }
        if(read_data(xvfs_header.files[pos].start_sector, data, xvfs_header.files[pos].size) == -1) {
            if(is_biffer_init) {
                delete[] data;
                data = NULL;
            }
            return -1;
        }
    }
    return xvfs_header.files[pos].size;
}

long xvfs::read_file(std::string vfs_file_name, char*& data) {
    if(!is_open_file) return -1;
    long long hash_vfs_file = calculate_crc64(vfs_file_name);
    return read_file(hash_vfs_file, data);
}

bool xvfs::delete_file(std::string vfs_file_name) {
    if(!is_open_file) return false;
    long long hash_vfs_file = calculate_crc64(vfs_file_name);
    return delete_file(hash_vfs_file);
}

bool xvfs::delete_file(long long hash_vfs_file) {
    if(!is_open_file) return false;
    long pos = binary_search_first(xvfs_header.files, hash_vfs_file, 0, xvfs_header.files.size() - 1);
    if(pos == -1) {
        return false;
    }
    if(!clear_data(xvfs_header.files[pos].start_sector, true)) return false;
    xvfs_header.files.erase(xvfs_header.files.begin() + pos);
    // сохраняем заголовок
    return save_header();
}

void xvfs::generate_table() {
    if(!is_svfs_crc64_table) {
        for(int i = 0; i < 256; ++i) {
            long long crc = i;
            for(int j=0; j<8; ++j) {
                if(crc & 1) {
                    crc >>= 1;
                    crc ^= poly;
                } else {
                    crc >>= 1;
                }
            }
            xvfs_crc64_table[i] = crc;
        }
        is_svfs_crc64_table = true;
    }
}

long long xvfs::calculate_crc64(long long crc, const unsigned char* stream, int n) {
    for(int i=0; i< n; ++i) {
        unsigned char index = stream[i] ^ crc;
        long long lookup = xvfs_crc64_table[index];
        crc >>= 8;
        crc ^= lookup;
    }
    return crc;
}

long long xvfs::calculate_crc64(std::string vfs_file_name) {
    return calculate_crc64(0, (const unsigned char*)vfs_file_name.c_str(), vfs_file_name.size());
}

long xvfs::binary_search_first(std::vector<_xvfs_file_header> arr, long long key, long left, long right) {
    if(arr.size() == 1) {
        if(arr[0].hash == key) return 0;
        return -1;
    }
    long midd = 0;
    while (true) {
        if(left > right)
            return -1;

        midd = (left + right) / 2;

        if(arr[midd].hash == key && (midd == 0 || arr[midd - 1].hash != key) ){
            return midd;
        } else if(arr[midd].hash == key) {
            right = midd - 1;
        } else if(arr[midd].hash > key) {
            right = midd - 1;
        } else {
            left = midd + 1;
        }
    }
    return -1;
}

void xvfs::init_xvfs_header() {
    xvfs_header.sector_size = 512;
    xvfs_header.files.resize(0);
    xvfs_header.empty_sectors.resize(0);
}

void xvfs::init_xvfs_header(int sector_size) {
    xvfs_header.sector_size = sector_size;
    xvfs_header.files.resize(0);
    xvfs_header.empty_sectors.resize(0);
}

bool xvfs::save_header() {
    //std::cout << "save_header" << std::endl;
    unsigned long header_size = xvfs_header.get_size();
    unsigned long files_size = xvfs_header.files.size();
    unsigned long empty_sectors_size = xvfs_header.empty_sectors.size();

    //std::cout << "header_size " << header_size << std::endl;
    //std::cout << "sector_size " << xvfs_header.sector_size << std::endl;
    //std::cout << "files_size " << files_size << std::endl;
    //std::cout << "empty_sectors_size " << empty_sectors_size << std::endl;

    char* buf = new char[header_size];
    memset(buf, 0, header_size);

    unsigned long offset = 0;
    // запишем размер заголовка
    std::memcpy(buf, &header_size, sizeof(header_size));
    offset += sizeof(header_size);
    // запищем размер сектора
    std::memcpy(buf + offset, &xvfs_header.sector_size, sizeof(xvfs_header.sector_size));
    offset += sizeof(xvfs_header.sector_size);
    // запишем количество файлов
    std::memcpy(buf + offset, &files_size, sizeof(files_size));
    offset += sizeof(files_size);
    // запишем сами файлы
    for(int i = 0; i < files_size; ++i) {
        _xvfs_file_header i_file_header = xvfs_header.files[i];
        std::memcpy(buf + offset, &i_file_header, sizeof(_xvfs_file_header));
        offset += sizeof(_xvfs_file_header);
    }
    // запишем количество пустых секторов
    std::memcpy(buf + offset, &empty_sectors_size, sizeof(empty_sectors_size));
    offset += sizeof(empty_sectors_size);
    // запишем сами пустые сектора
    for(int i = 0; i < empty_sectors_size; ++i) {
        std::memcpy(buf + offset, &xvfs_header.empty_sectors[i], sizeof(unsigned long));
        offset += sizeof(unsigned long);
    }
    if(write_data(0, buf, header_size) == -1) {
        delete[] buf;
        return false;
    }
    delete[] buf;
    return true;
}
