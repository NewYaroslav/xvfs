#include "xvfs.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>

#ifdef XFVS_USE_MINLIZO

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);

#endif

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

xvfs::~xvfs() {
    fvs_file.close();
}

xvfs::xvfs(std::string file_name) {
    xvfs::file_name = file_name; // запоминаем имя файла виртуальноц файловой системы
    generate_table(); // инициализируем crc таблицу
    if(!check_file(file_name)) {
        is_open_file = create_file(file_name);
        if(is_open_file) {
            fvs_file = std::fstream(file_name, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
            if (!fvs_file.is_open()) {
                is_open_file = false;
                return; // не смогли октрыть файл
            }
            init_xvfs_header(512, NO_COMPRESSION);
            save_header();
        }
    } else {
        fvs_file = std::fstream(file_name, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
        if (!fvs_file.is_open()) {
            is_open_file = false;
            return; // не смогли октрыть файл
        }
        is_open_file = read_header();
#       ifdef XFVS_USE_MINLIZO
        if(is_open_file && xvfs_header.compression_type == USE_MINLIZO && lzo_init() != LZO_E_OK) {
            //std::cout << "lzo_init() != LZO_E_OK" << std::endl;
            is_open_file = false;
        }
#       endif
    }
}

xvfs::xvfs(std::string file_name, int sector_size) {
    const long min_sector_size = 32;
    xvfs::file_name = file_name; // запоминаем имя файла виртуальноц файловой системы
    generate_table(); // инициализируем crc таблицу
    if(!check_file(file_name)) {
        //std::cout << "!check_file" << std::endl;
        if(sector_size < min_sector_size) {
            is_open_file = false;
            return;
        }
        is_open_file = create_file(file_name);
        if(is_open_file) {
            fvs_file = std::fstream(file_name, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
            if (!fvs_file.is_open()) {
                is_open_file = false;
                return; // не смогли октрыть файл
            }
            init_xvfs_header(sector_size, NO_COMPRESSION);
            save_header();
        }
    } else {
        //std::cout << "check_file" << std::endl;
        fvs_file = std::fstream(file_name, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
        if (!fvs_file.is_open()) {
            is_open_file = false;
            return; // не смогли октрыть файл
        }
        is_open_file = read_header();
#       ifdef XFVS_USE_MINLIZO
        if(is_open_file && xvfs_header.compression_type == USE_MINLIZO && lzo_init() != LZO_E_OK) {
            //std::cout << "lzo_init() != LZO_E_OK" << std::endl;
            is_open_file = false;
        }
#       endif
    }
}

xvfs::xvfs(std::string file_name, int sector_size, int compression_type) {
    const long min_sector_size = 32;
    xvfs::file_name = file_name; // запоминаем имя файла виртуальноц файловой системы
    generate_table(); // инициализируем crc таблицу
    if(!check_file(file_name)) {
        //std::cout << "!check_file" << std::endl;
        if(sector_size < min_sector_size || compression_type < 0 || (compression_type > USE_ZLIB_LEVEL_9 && compression_type != USE_MINLIZO && compression_type != USE_LZ4)) {
            is_open_file = false;
            return;
        }
        is_open_file = create_file(file_name);
        if(is_open_file) {
            fvs_file = std::fstream(file_name, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
            if (!fvs_file.is_open()) {
                is_open_file = false;
                return; // не смогли октрыть файл
            }
#           ifdef XFVS_USE_MINLIZO
            if(compression_type == USE_MINLIZO && lzo_init() != LZO_E_OK) {
                //std::cout << "lzo_init() != LZO_E_OK" << std::endl;
                is_open_file = false;
                return;
            }
#           endif
            init_xvfs_header(sector_size, compression_type);
            save_header();
        }
    } else {
        //std::cout << "check_file" << std::endl;
        fvs_file = std::fstream(file_name, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
        if (!fvs_file.is_open()) {
            is_open_file = false;
            return; // не смогли октрыть файл
        }
        is_open_file = read_header();
#       ifdef XFVS_USE_MINLIZO
        if(is_open_file && xvfs_header.compression_type == USE_MINLIZO && lzo_init() != LZO_E_OK) {
            //std::cout << "lzo_init() != LZO_E_OK" << std::endl;
            is_open_file = false;
        }
#       endif
    }
}

bool xvfs::read_header() {
    if (!fvs_file.is_open()) {
        return false; // не смогли октрыть файл
    }
    fvs_file.seekg(0, std::ios::end);
    unsigned long file_size = fvs_file.tellg();
    fvs_file.seekg(0, std::ios::beg);
    fvs_file.clear();

    unsigned long header_size = 0;
    // читаем размер
    fvs_file.read(reinterpret_cast<char *>(&header_size),sizeof (header_size));
    //std::cout << "header_size " << header_size << std::endl;
    //std::cout << "file_size " << file_size << std::endl;
    if(header_size > file_size) {
        return false;
    }
    // читаем размер сектора
    fvs_file.read(reinterpret_cast<char *>(& xvfs_header.sector_size),sizeof ( xvfs_header.sector_size));
    // читаем тип компресии
    fvs_file.read(reinterpret_cast<char *>(& xvfs_header.compression_type),sizeof ( xvfs_header.compression_type));
    //std::cout << "sector_size " << xvfs_header.sector_size << std::endl;

#   if !(defined(XFVS_USE_ZLIB) || defined(XFVS_USE_MINLIZO) || defined(XFVS_USE_LZ4))
    if(xvfs_header.compression_type != NO_COMPRESSION) {
        return false;
    }
#   endif

    const unsigned long min_sector_size = 32;
    if(xvfs_header.sector_size < min_sector_size) {
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
        fvs_file.seekg(next_pos, std::ios::beg);
        fvs_file.read(buf, xvfs_header.sector_size);
        if(!fvs_file) {
            delete[] buf;
            delete[] header_data;
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
    }
    if(len != header_size) {
        delete[] buf;
        delete[] header_data;
        //std::cout << "error len != header_size" << std::endl;
        return false;
    }
    // парсим header_data
    unsigned long files_size = 0;
    unsigned long offset = sizeof (header_size) + sizeof (xvfs_header.sector_size) + sizeof (xvfs_header.compression_type);
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
    if(!is_open_file) return ERROR_VFS_FILE_NOT_OPEN;

    unsigned long next_sector = start_sector;
    unsigned long next_pos = start_sector * xvfs_header.sector_size; // получаем начальный сектор
    unsigned long len = 0;
    char* buf = new char[xvfs_header.sector_size];
    while(next_sector != 0xFFFFFFFF) {
        fvs_file.seekg(next_pos, std::ios::beg);
        if(!fvs_file) {
            ///fvs_file.close();
            delete[] buf;
            return ERROR_VFS_READING_FILE;
        }
        fvs_file.read(buf, xvfs_header.sector_size);
        if(!fvs_file) {
            ///fvs_file.close();
            delete[] buf;
            return ERROR_VFS_READING_FILE;
        }
        unsigned long old_len = len;
        len += xvfs_header.sector_size - sizeof(unsigned long);
        if(len > file_size) len = file_size;
        std::memcpy(file_data + old_len, buf, len - old_len);
        if(len == file_size) break;
        next_sector = ((unsigned long*)(buf + (xvfs_header.sector_size - sizeof(unsigned long))))[0];
        next_pos = next_sector * xvfs_header.sector_size;
    }
    ///fvs_file.close();
    delete[] buf;
    return len;
}

long xvfs::write_data(unsigned long start_sector, char* file_data, unsigned long file_size) {
    if(!is_open_file) return ERROR_VFS_FILE_NOT_OPEN;

    unsigned long next_sector = start_sector;
    unsigned long next_pos = start_sector * xvfs_header.sector_size; // получаем начальный сектор
    unsigned long len = 0;

    fvs_file.seekg (0, std::ios::end);
    unsigned long real_file_size = fvs_file.tellg();
    fvs_file.seekg (next_pos, std::ios::beg);
    fvs_file.clear();

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
                fvs_file.seekg(offset_sector, std::ios::beg);
                fvs_file.clear();
                fvs_file.read(reinterpret_cast<char *>(&next_sector),sizeof(unsigned long));
                fvs_file.seekg(next_pos, std::ios::beg);
                //std::cout << "read next_sector " << next_sector << std::endl;
                if(next_sector == 0xFFFFFFFF) {
                    // выбираем один из пустых секторов
                    // или перемещаемся в конец файла
                    if(xvfs_header.empty_sectors.size() > 0) {
                        next_sector = xvfs_header.empty_sectors[0];
                        xvfs_header.empty_sectors.erase(xvfs_header.empty_sectors.begin());
                    } else {
                        fvs_file.seekg(0, std::ios::end);
                        unsigned long _size = fvs_file.tellg();
                        next_sector = _size / xvfs_header.sector_size;
                    }
                }
            } else {
                is_end_file = true;
                next_sector = (real_file_size / xvfs_header.sector_size) + 1;
            }
        } else {
            next_sector++;
        }

        std::memcpy(buf, file_data + old_len, len - old_len);
        std::memcpy(buf + xvfs_header.sector_size - sizeof(unsigned long), &next_sector, sizeof(unsigned long));

        fvs_file.seekg (next_pos, std::ios::beg);
        fvs_file.write(buf, xvfs_header.sector_size);

        if(!fvs_file) {
            delete[] buf;
            //std::cout << "ERROR_VFS_WRITING_FILE" << std::endl;
            return ERROR_VFS_WRITING_FILE;
        }
        if(len == file_size) break;
        next_pos = next_sector * xvfs_header.sector_size;
    }

    delete[] buf;
    return len;
}

bool xvfs::clear_data(unsigned long start_sector, bool is_first_sector) {
    if(!is_open_file) return false; // файл не был открыт
    if(start_sector == 0xFFFFFFFF) return true; // если это последний сектор, просто выходим
    //std::fstream fout(file_name, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
    if (!fvs_file.is_open()) {
        return false; // не смогли октрыть файл
    }
    unsigned long next_sector = start_sector;
    unsigned long next_pos = start_sector * xvfs_header.sector_size; // получаем начальный сектор
    while(1) {
        // узнаем номер следующего сектора
        unsigned long old_next_sector = next_sector;
        fvs_file.seekg(next_pos + xvfs_header.sector_size - sizeof(unsigned long), std::ios::beg);
        fvs_file.read(reinterpret_cast<char *>(&next_sector),sizeof(unsigned long));

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
        fvs_file.seekg(next_pos + xvfs_header.sector_size - sizeof(unsigned long), std::ios::beg);
        unsigned long END_SECTOR = 0xFFFFFFFF;
        fvs_file.clear();
        fvs_file.write(reinterpret_cast<char *>(&END_SECTOR),sizeof(unsigned long));
        next_pos = next_sector * xvfs_header.sector_size;
    }
    return true;
}

unsigned long xvfs::get_last_new_sector() {
    fvs_file.seekg(0, std::ios::end);
    unsigned long _size = fvs_file.tellg();
    unsigned long last_sector = _size / xvfs_header.sector_size;
    return last_sector;
}

bool xvfs::write_file(long long hash_vfs_file, char* _data, unsigned long _len) {
    if(!is_open_file) return false;
    long pos = binary_search_first(xvfs_header.files, hash_vfs_file, 0, xvfs_header.files.size() - 1);

    if(pos == -1 && _len == 0) { // если файл пустой, просто сохраним данные в заголовке
        // добавим файл в заголовок
        _xvfs_file_header i_file_header(hash_vfs_file, 0, 0xFFFFFFFF, 0);

        if(xvfs_header.files.size() > 0) {
            auto it = std::lower_bound(xvfs_header.files.begin(), xvfs_header.files.end(), i_file_header);
            xvfs_header.files.insert(it, i_file_header);
        } else {
            xvfs_header.files.push_back(i_file_header);
        } // if
        // сохраняем заголовок
        return save_header();
    }

    unsigned long start_sector;
#   if defined(XFVS_USE_ZLIB) || defined(XFVS_USE_MINLIZO) || defined(XFVS_USE_LZ4)
    unsigned long len;
    char* data = NULL;
    if(xvfs_header.compression_type == NO_COMPRESSION) {
        data = _data;
        len = _len;
    } else
#   if defined(XFVS_USE_ZLIB)
    if(xvfs_header.compression_type <= USE_ZLIB_LEVEL_9) {
        len = _len + 0.01 *_len + 12;
        data = new char[len];
        if(compress2((unsigned char*)data, &len, (const unsigned char*)_data, _len, xvfs_header.compression_type) != Z_OK) {
            delete[] data;
            return false;
        }
        //std::cout << "compress2 " << len << " old size " << _len << " type " << xvfs_header.compression_type << std::endl;
    } else
#   endif
#   if defined(XFVS_USE_MINLIZO)
    if(xvfs_header.compression_type == USE_MINLIZO) {
        len = _len + _len / 16 + 64 + 3;
        data = new char[len];
        unsigned long long lzo_len = len;
        int r = lzo1x_1_compress((const unsigned char*)_data, _len, (unsigned char*)data, &lzo_len, wrkmem);
        len = lzo_len;
        if(r != LZO_E_OK) {
            delete[] data;
            return false;
        }
    } else
#   endif
#   if defined(XFVS_USE_LZ4)
    if(xvfs_header.compression_type == USE_LZ4) {
        len = LZ4_compressBound(_len);
        //std::cout << "lz4 len " << len << std::endl;
        data = new char[len];
        int compressed_data_size = LZ4_compress_default(_data, data, _len, len);
        if(compressed_data_size <= 0) {
            //std::cout << "compressed_data_size " << compressed_data_size << std::endl;
            delete[] data;
            return false;
        }
        len = compressed_data_size;
    } else
#   endif
    {
        return false;
    }
#   else
    char* data = _data;
    unsigned long& len = _len;
#   endif
    if(pos == -1) { // если файла нет
        if(xvfs_header.empty_sectors.size() > 0) { // если есть пустые сектора
            start_sector = xvfs_header.empty_sectors[0];
            xvfs_header.empty_sectors.erase(xvfs_header.empty_sectors.begin());
        } else { // если пустых секторов нет
            start_sector = get_last_new_sector(); // последний новый сектор
        }
        // пишем файл
        if(write_data(start_sector, data, len) < 0) {
#           if defined(XFVS_USE_ZLIB) || defined(XFVS_USE_MINLIZO) || defined(XFVS_USE_LZ4)
            if(xvfs_header.compression_type != NO_COMPRESSION) delete[] data;
#           endif
            return false;
        }
        // добавим файл в заголовок
        _xvfs_file_header i_file_header(hash_vfs_file, len, start_sector, _len);

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
            if(!clear_data(start_sector, false)) {
#               if defined(XFVS_USE_ZLIB) || defined(XFVS_USE_MINLIZO) || defined(XFVS_USE_LZ4)
                if(xvfs_header.compression_type != NO_COMPRESSION) delete[] data;
#               endif
                return false;
            }
            if(write_data(start_sector, data, len) < 0) {
#               if defined(XFVS_USE_ZLIB) || defined(XFVS_USE_MINLIZO) || defined(XFVS_USE_LZ4)
                if(xvfs_header.compression_type != NO_COMPRESSION) delete[] data;
#               endif
                return false;
            }
        } else { // если длина файла совпадает
            // запишем данные туда же
            if(write_data(start_sector, data, len) < 0) {
#               if defined(XFVS_USE_ZLIB) || defined(XFVS_USE_MINLIZO) || defined(XFVS_USE_LZ4)
                if(xvfs_header.compression_type != NO_COMPRESSION) delete[] data;
#               endif
                return false;
            }
        } //
        xvfs_header.files[pos] = _xvfs_file_header(hash_vfs_file, len, start_sector, _len);
    } //
#   if defined(XFVS_USE_ZLIB) || defined(XFVS_USE_MINLIZO) || defined(XFVS_USE_LZ4)
    if(xvfs_header.compression_type != NO_COMPRESSION) delete[] data;
#   endif
    // сохраняем заголовок
    return save_header();
}

bool xvfs::write_file(std::string vfs_file_name, char* data, unsigned long len) {
    if(!is_open_file) return false;
    long long hash_vfs_file = calculate_crc64(vfs_file_name);
    return write_file(hash_vfs_file, data, len);
}

long xvfs::get_len_file(long long hash_vfs_file) {
    if(!is_open_file) return ERROR_VFS_FILE_NOT_OPEN;
    long pos = binary_search_first(xvfs_header.files, hash_vfs_file, 0, xvfs_header.files.size() - 1);
    if(pos == -1) {
        return ERROR_VIRTUAL_FILE_NOT_FOUND;
    }
    return xvfs_header.files[pos].size;
}

long xvfs::get_len_file(std::string vfs_file_name) {
    if(!is_open_file) return ERROR_VFS_FILE_NOT_OPEN;
    long long hash_vfs_file = calculate_crc64(vfs_file_name);
    return get_len_file(hash_vfs_file);
}

long xvfs::read_file(long long hash_vfs_file, char*& data) {
    if(!is_open_file) return ERROR_VFS_FILE_NOT_OPEN;
    long pos = binary_search_first(xvfs_header.files, hash_vfs_file, 0, xvfs_header.files.size() - 1);
    if(pos == -1) {
        return ERROR_VIRTUAL_FILE_NOT_FOUND;
    } else {
        if(xvfs_header.files[pos].real_size == 0) {
            data = NULL;
            return 0;
        }
#       if !(defined(XFVS_USE_ZLIB) || defined(XFVS_USE_MINLIZO) || defined(XFVS_USE_LZ4))
        bool is_biffer_init = false;
        if(data == NULL) {
            data = new char[xvfs_header.files[pos].size];
            is_biffer_init = true;
        }
        long errData = (read_data(xvfs_header.files[pos].start_sector, data, xvfs_header.files[pos].size);
        if(errData < 0) {
            if(is_biffer_init) {
                delete[] data;
                data = NULL;
            }
            return errData;
        }
#       else
        if(xvfs_header.compression_type == NO_COMPRESSION) {
            bool is_biffer_init = false;
            if(data == NULL) {
                data = new char[xvfs_header.files[pos].size];
                is_biffer_init = true;
            }
            long errData = read_data(xvfs_header.files[pos].start_sector, data, xvfs_header.files[pos].size);
            if(errData < 0) {
                if(is_biffer_init) {
                    delete[] data;
                    data = NULL;
                }
                return errData;
            }
        } else
#       if defined(XFVS_USE_ZLIB)
        if(xvfs_header.compression_type <= USE_ZLIB_LEVEL_9) {
            bool is_biffer_init = false;
            unsigned long real_size = xvfs_header.files[pos].real_size;
            char* raw_data = new char[xvfs_header.files[pos].size];
            if(data == NULL) {
                data = new char[real_size];
                is_biffer_init = true;
            }
            // читаем сырые данные
            long errData = read_data(xvfs_header.files[pos].start_sector, raw_data, xvfs_header.files[pos].size);
            if(errData < 0) {
                if(is_biffer_init) {
                    delete[] data;
                    data = NULL;
                }
                delete[] raw_data;
                //std::cout << "err read_data" << std::endl;
                return errData;
            }
            // декомпрессия сырых данных
            int err_uncompress = uncompress((unsigned char*)data, &real_size, (unsigned char*)raw_data, xvfs_header.files[pos].size);

            if(err_uncompress != Z_OK) {
                if(is_biffer_init) {
                    delete[] data;
                    data = NULL;
                }
                delete[] raw_data;
                std::cout << "err uncompress " << err_uncompress << std::endl;
                return ERROR_VIRTUAL_FILE_DECOMPRESSION;
            }
            delete[] raw_data;
            //std::cout << "real_size " << real_size << " xvfs_header real_size " << xvfs_header.files[pos].real_size << std::endl;
            return real_size;
        } else
#       endif
#       if defined(XFVS_USE_MINLIZO)
        if(xvfs_header.compression_type == USE_MINLIZO) {
            bool is_biffer_init = false;
            unsigned long long real_size = xvfs_header.files[pos].real_size;
            char* raw_data = new char[xvfs_header.files[pos].size];
            if(data == NULL) {
                data = new char[real_size];
                is_biffer_init = true;
            }
            // читаем сырые данные
            long errData = read_data(xvfs_header.files[pos].start_sector, raw_data, xvfs_header.files[pos].size);
            if(errData < 0) {
                if(is_biffer_init) {
                    delete[] data;
                    data = NULL;
                }
                delete[] raw_data;
                //std::cout << "err read_data" << std::endl;
                return errData;
            }
            // декомпрессия сырых данных
            int r = lzo1x_decompress((unsigned char*)raw_data, xvfs_header.files[pos].size,(unsigned char*)data, &real_size, NULL);
            if(r != LZO_E_OK || real_size != xvfs_header.files[pos].real_size) {
                if(is_biffer_init) {
                    delete[] data;
                    data = NULL;
                }
                delete[] raw_data;
                //std::cout << "r != LZO_E_OK || real_size != xvfs_header.files[pos].real_size " << std::endl;
                return ERROR_VIRTUAL_FILE_DECOMPRESSION;
            }
            delete[] raw_data;
            //std::cout << "real_size " << real_size << " xvfs_header real_size " << xvfs_header.files[pos].real_size << std::endl;
            return real_size;
        } else
#       endif
#       if defined(XFVS_USE_LZ4)
        if(xvfs_header.compression_type == USE_LZ4) {
            bool is_biffer_init = false;
            unsigned long real_size = xvfs_header.files[pos].real_size;
            char* raw_data = new char[xvfs_header.files[pos].size];
            if(data == NULL) {
                data = new char[real_size];
                is_biffer_init = true;
            }
            // читаем сырые данные
            long errData = read_data(xvfs_header.files[pos].start_sector, raw_data, xvfs_header.files[pos].size);
            if(errData < 0) {
                if(is_biffer_init) {
                    delete[] data;
                    data = NULL;
                }
                delete[] raw_data;
                return errData;
            }
            // декомпрессия сырых данных
            const int decompressed_size = LZ4_decompress_safe(raw_data, data, xvfs_header.files[pos].size, real_size);

            if(decompressed_size <= 0) {
                if(is_biffer_init) {
                    delete[] data;
                    data = NULL;
                }
                delete[] raw_data;
                std::cout << "decompressed_size " << decompressed_size << std::endl;
                return ERROR_VIRTUAL_FILE_DECOMPRESSION;
            }
            delete[] raw_data;
            //std::cout << "real_size " << real_size << " xvfs_header real_size " << xvfs_header.files[pos].real_size << std::endl;
            return real_size;
        } else
#       endif
        {
            data = NULL;
            return ERROR_UNKNOWN_DECOMPRESSION_METHOD;
        }
#       endif
    }
    return xvfs_header.files[pos].real_size;
}

long xvfs::read_file(std::string vfs_file_name, char*& data) {
    if(!is_open_file) return ERROR_VFS_FILE_NOT_OPEN;
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


void xvfs::init_xvfs_header(int sector_size, int compression_type) {
    xvfs_header.sector_size = sector_size;
    xvfs_header.compression_type = compression_type;
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
    // запищем тип компресии файлов
    std::memcpy(buf + offset, &xvfs_header.compression_type, sizeof(xvfs_header.compression_type));
    offset += sizeof(xvfs_header.compression_type);
    // запишем количество файлов
    std::memcpy(buf + offset, &files_size, sizeof(files_size));
    offset += sizeof(files_size);
    // запишем сами файлы
    for(unsigned long i = 0; i < files_size; ++i) {
        _xvfs_file_header i_file_header = xvfs_header.files[i];
        std::memcpy(buf + offset, &i_file_header, sizeof(_xvfs_file_header));
        offset += sizeof(_xvfs_file_header);
    }
    // запишем количество пустых секторов
    std::memcpy(buf + offset, &empty_sectors_size, sizeof(empty_sectors_size));
    offset += sizeof(empty_sectors_size);
    // запишем сами пустые сектора
    for(unsigned long i = 0; i < empty_sectors_size; ++i) {
        std::memcpy(buf + offset, &xvfs_header.empty_sectors[i], sizeof(unsigned long));
        offset += sizeof(unsigned long);
    }
    if(write_data(0, buf, header_size) < 0) {
        delete[] buf;
        return false;
    }
    delete[] buf;
    return true;
}

bool xvfs::open(long long hash_vfs_file, int mode) {
    if(!is_open_file) return false;
    open_mode = mode; // запоминаем режим
    open_hash = hash_vfs_file; // запоминаем hash файла
    open_size = 0;
    open_pos = 0;
    if(open_buffer_write != NULL) {
        delete[] open_buffer_write;
        open_buffer_write = NULL;
    }
    if(open_mode == READ_FILE) {
        if(open_buffer_read != NULL) {
            delete[] open_buffer_read;
            open_buffer_read = NULL;
        }
        open_size = read_file(hash_vfs_file, open_buffer_read);
        if(open_size < 0) {
            open_mode = -1;
            return false;
        }
    } else if(open_mode != WRITE_FILE) {
        open_mode = -1;
        return false;
    }
    return true;
}

bool xvfs::open(std::string vfs_file_name, int mode) {
    if(!is_open_file) return false;
    long long hash_vfs_file = calculate_crc64(vfs_file_name);
    return open(hash_vfs_file, mode);
}

bool xvfs::reserve_memory(unsigned long size) {
    if(open_buffer_write == NULL && open_mode == WRITE_FILE) {
        open_buffer_write = new char[size];
        open_size = size;
        return true;
    }
    open_size = 0;
    return false;
}

long xvfs::get_size() {
    if(open_mode == READ_FILE || open_mode == WRITE_FILE) return open_size;
    return ERROR_VIRTUAL_FILE_NOT_OPEN;
}

long xvfs::write(void* data, long size) {
    if(open_mode != WRITE_FILE) return ERROR_VIRTUAL_FILE_NOT_OPEN;
    if(open_pos >= open_size) {
        //std::cout << "(open_pos + size) >= open_size" << std::endl;
        //std::cout << "open_pos " << open_pos << std::endl;
        //std::cout << "open_size " << open_size << std::endl;
        return 0;
    }
    std::memcpy(open_buffer_write + open_pos, data, size);
    open_pos += size;
    //std::cout << "open_pos " << open_pos << std::endl;
    return size;
}

long xvfs::read(void* data, long size) {
    if(open_mode != READ_FILE) return ERROR_VIRTUAL_FILE_NOT_OPEN;
    char* dataPtr = static_cast<char*>(data);
    for(long i = 0; i < size; ++i) {
        long pos = open_pos + i;
        if(pos >= open_size) return i;
        dataPtr[i] = open_buffer_read[pos];
    }
    open_pos += size;
    return size;
}

bool xvfs::close() {
    if(open_mode == READ_FILE) {
        if(open_buffer_read != NULL) {
            delete[] open_buffer_read;
            open_buffer_read = NULL;
        }
        open_mode = -1;
        return true;
    } else
    if(open_mode == WRITE_FILE) {
        if(!write_file(open_hash, open_buffer_write, open_pos)) {
            if(open_buffer_write != NULL) {
                delete[] open_buffer_write;
                open_buffer_write = NULL;
            }
            open_mode = -1;
            return false;
        }
        if(open_buffer_write != NULL) {
            delete[] open_buffer_write;
            open_buffer_write = NULL;
        }
        open_mode = -1;
        return true;
    }
    open_mode = -1;
    return false;
}
