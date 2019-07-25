#ifndef XVFS_HPP_INCLUDED
#define XVFS_HPP_INCLUDED

#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

/** \brief Класс виртуальной файловой системы
 */
class xvfs {
public:
    enum xfvs_errors {
        OK = 0,
        VFS_FILE_READ_ERROR = -1,
        VFS_FILE_WRITE_ERROR = -2,
    };

    enum {
        END_SECTOR = 0xFFFFFFFF,
        HEADER_START_SECTOR = 0x00000000,
    };
//private:
public:
    std::fstream fvs_file;                      /**< Файл виртуальной файловой системы */
    bool is_fvs_file_open = false;

    inline bool fvs_create_file(std::string file_name) {
        std::fstream file(file_name, std::ios::out | std::ios::app);
        if(!file) return false;
        file.close();
        return true;
    }

    inline bool fvs_check_file(std::string file_name) {
        std::ifstream file;
        file.open(file_name);
        if(!file) return false;
        file.close();
        return true;
    }

    inline bool fvs_open_file(std::string file_name) {
        if(is_fvs_file_open) fvs_file.close();
        fvs_file = std::fstream(file_name, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
        if (!fvs_file.is_open()) {
            is_fvs_file_open = false;
            return false;
        }
        is_fvs_file_open = true;
        return true;
    }

    /** \brief Смещение относительно начала файла виртуальной системы
     * \param file_offset смещение в файле
     * \return вернет true в случае успеха
     */
    inline bool fvs_seekg_beg(const unsigned long &file_offset) {
        if(!fvs_file.seekg(file_offset, std::ios::beg)) return false;
        fvs_file.clear();
        return true;
    }

    inline bool fvs_seekg_end(const unsigned long &file_offset) {
        if(!fvs_file.seekg(file_offset, std::ios::end)) return false;
        fvs_file.clear();
        return true;
    }

    /** \brief Записать данные сектора
     * Данная функция записывает ТОЛЬКО данные! Она не записывает ссылку на следующий сектор!
     * \param buffer буфер с данными
     * \param length длина буфера с данными
     * \return вернет true в случае успеха
     */
    inline bool fvs_write_data(char* buffer, const unsigned long &length) {
        if(length == vfs_header.sector_data_size) {
            if(!fvs_file.write(buffer, length)) return false;
        } else {
            if(!fvs_file.write(buffer, length)) return false;
            unsigned long null_buffer_length = vfs_header.sector_data_size - length;
            char *null_buffer = new char(null_buffer_length);
            std::fill(null_buffer, null_buffer + null_buffer_length, '\0');
            if(!fvs_file.write(null_buffer, null_buffer_length)) {
                delete null_buffer;
                return false;
            }
            delete null_buffer;
        }
        return true;
    }

    inline bool fvs_write_end(char* buffer, const unsigned long &length) {
        if(!fvs_seekg_end(0)) return false;
        if(!fvs_write_data(buffer, length)) return false;
        return true;
    }

    /** \brief Данная функция записывает сектор в конец файла
     * \param buffer буфер с данными
     * \param length длина буфера с данными
     * \param is_end флаг последнего пакета данных
     * \return вернет true в случае успеха
     */
    inline bool fvs_write_data_to_end(unsigned long &next_sector, char* buffer, const unsigned long &length, const bool &is_end) {
        // пишем в новый сектор сектор
        if(!fvs_write_end(buffer, length)) return false;
        if(is_end) next_sector = END_SECTOR;
        else next_sector = vfs_header.get_sectors() + 1;
        fvs_file.write(reinterpret_cast<char *>(&next_sector), sizeof(unsigned long));
        if(!fvs_file) {
            return false;
        }
        vfs_header.vfs_file_size += vfs_header.sector_size;
        return true;
    }

    /** \brief Получить следующий секор
     * Данная функция узнает следующий сектор секор
     * \param file_offset смещение в файле
     * \param next_sector следующий секор
     * \return вернет 0 в случае успеха
     */
    inline int fvs_get_next_sector(unsigned long file_offset, unsigned long &next_sector) {
        if(!fvs_seekg_beg(file_offset + vfs_header.sector_data_size)) return VFS_FILE_READ_ERROR;
        if(!fvs_file.read(reinterpret_cast<char *>(&next_sector), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
        return OK;
    }

    /** \brief Установить следующий секор
     * Данная функция перезаписывает секор
     * \param file_offset смещение в файле
     * \param next_sector следующий секор
     * \return вернет 0 в случае успеха
     */
    inline int fvs_set_next_sector(unsigned long file_offset, unsigned long next_sector) {
        if(!fvs_seekg_beg(file_offset + vfs_header.sector_data_size)) return VFS_FILE_READ_ERROR;
        if(!fvs_file.write(reinterpret_cast<char *>(&next_sector), sizeof(unsigned long))) return VFS_FILE_WRITE_ERROR;
        return OK;
    }

    inline unsigned long fvs_get_file_size(unsigned long pos) {
        fvs_file.seekg(0, std::ios::end);
        return fvs_file.tellg();
    }

    /** \brief Структура заголовка виртуального файла
     */
    class header_file {
    public:
        unsigned long long unique_id;           /**< Уникальный ID файkа */
        unsigned long file_size;                /**< Размер файла */
        unsigned long start_sector;             /**< Сектор начала данных */

        header_file() {};

        bool operator<(const header_file& value)const{return unique_id < value.unique_id;}

        const unsigned long get_size() {
            return sizeof(unsigned long long) + sizeof(unsigned long) + sizeof(unsigned long);
        }
    };

    /** \brief Заголовок виртуального файла
     */
    class header {
    public:
        unsigned long sector_size = 0;                  /**< Размер сектора */
        unsigned long sector_data_size = 0;             /**< Размер данных сектора */
        unsigned long vfs_file_size = 0;                /**< Размер файла виртуальной файловой системы */
        std::vector<header_file> files;                 /**< Файлы */
        std::vector<unsigned long> empty_sectors;       /**< Пустые сектора */

        header() {};

        void init(std::fstream &fvs_file, unsigned long sector_size) {
            header::sector_size = sector_size;
            header::sector_data_size = sector_size - sizeof(unsigned long);
            fvs_file.seekg(0, std::ios::end);
            vfs_file_size = fvs_file.tellg();
        }

        /** \brief Получить смещение в файле виртуальной файловой системы
         * \param sector сектор
         * \return смещение в файле в байтах
         */
        inline unsigned long get_offset(unsigned long sector) {
            return sector * sector_size;
        }

        inline unsigned long get_sectors() {
            return vfs_file_size / sector_size;
        }

        unsigned long get_size() {
            return 3*sizeof(unsigned long) + get_size()*files.size() + sizeof(unsigned long)*empty_sectors.size();
        }
    };// xvfs_header;

    header vfs_header = header();

    inline bool check_offset(const unsigned long &file_offset) {
        if((file_offset + vfs_header.sector_size) > vfs_header.vfs_file_size) return false;
        return true;
    }

    /** \brief Прочитать сектор в буффер
     * \param sector сектор
     * \param buffer буффер
     * \param length длина буффера
     * \return вернет 0 в случае успеха, иначе код ошибки
     */
    int read_sector(
            unsigned long &sector,
            char *buffer,
            unsigned long length) {
        unsigned long file_offset = vfs_header.get_offset(sector);
        if(!check_offset(file_offset)) return VFS_FILE_READ_ERROR;
        if(!fvs_seekg_beg(file_offset)) return VFS_FILE_READ_ERROR;
        if(!fvs_file.read(buffer, length)) return VFS_FILE_READ_ERROR;
        if(fvs_get_next_sector(file_offset, sector) != OK) return VFS_FILE_READ_ERROR;
        return OK;
    }

    /** \brief Прочитать сектора в буффер
     * \param start_sector начальный сектор чтения
     * \param buffer буффер
     * \param length длина буффера
     * \return вернет 0 в случае успеха, иначе код ошибки
     */
    int read_sectors(unsigned long start_sector, char *buffer, unsigned long length) {
        unsigned long read_bytes = 0;
        while(length > 0) {
            if(read_sector(
                start_sector,
                buffer + read_bytes,
                length >= vfs_header.sector_data_size ?
                    vfs_header.sector_data_size : length) != OK) {
                return VFS_FILE_READ_ERROR;
            }
            length -= vfs_header.sector_data_size;
            read_bytes += vfs_header.sector_data_size;
            if(length > 0 && start_sector == END_SECTOR) return VFS_FILE_READ_ERROR;
        }
        return OK;
    }

    int write_sector(
            unsigned long &sector,
            char *buffer,
            unsigned long length,
            bool is_end = false) {
        unsigned long file_offset = vfs_header.get_offset(sector);
        if(!check_offset(file_offset)) {
            // пишем в новый сектор сектор
            std::cout << "new sector " << sector << std::endl;
            std::cout << "file_offset " << file_offset << std::endl;
            std::cout << "vfs_header.sector_size " << vfs_header.sector_size << std::endl;
            std::cout << "vfs_header.vfs_file_size " << vfs_header.vfs_file_size << std::endl;
            if(!fvs_write_data_to_end(sector, buffer, length, is_end)) return VFS_FILE_WRITE_ERROR;
        } else {
            std::cout << "old sector " << sector << std::endl;
            // пишем в существующий сектор
            if(!fvs_seekg_beg(file_offset)) return VFS_FILE_READ_ERROR;
            if(!fvs_file.write(buffer, length)) return VFS_FILE_WRITE_ERROR;
            if(fvs_get_next_sector(file_offset, sector) != OK) return VFS_FILE_READ_ERROR;

            if(is_end && sector != END_SECTOR) {
                // была ссылка на следующий секор

                // добавим пустой сектор в список пустых секторов
                if(vfs_header.empty_sectors.size() == 0) {
                    // добавим пустой сектор в список пустых секторов
                    vfs_header.empty_sectors.push_back(sector);
                } else {
                    auto empty_sectors_it = std::upper_bound(
                        vfs_header.empty_sectors.begin(),
                        vfs_header.empty_sectors.end(),
                        sector);
                    if(*empty_sectors_it != sector) {
                        vfs_header.empty_sectors.insert(empty_sectors_it, sector);
                    }
                }
                sector = END_SECTOR;
                fvs_set_next_sector(file_offset, sector);
            } else
            if(!is_end && sector == END_SECTOR) {
                // необхоимо прописать новый сектор
                if(vfs_header.empty_sectors.size() > 0) {
                    sector = vfs_header.empty_sectors[0];
                    vfs_header.empty_sectors.erase(vfs_header.empty_sectors.begin());
                    fvs_set_next_sector(file_offset, sector);
                } else {
                    sector = vfs_header.get_sectors();
                    fvs_set_next_sector(file_offset, sector);
                } // if
            } // if
        } // if
        return OK;
    }

    /** \brief Записать в сектора из буффера
     * \param start_sector начальный сектор записи
     * \param buffer буффер
     * \param length длина буффера
     * \return вернет 0 в случае успеха, иначе код ошибки
     */
    int write_sectors(unsigned long start_sector, char *buffer, unsigned long length) {
        unsigned long written_bytes = 0;
        while(length > 0) {
            // находим длину фрагмента данных
            unsigned long fragment_data_length =
                length >= vfs_header.sector_data_size ? vfs_header.sector_data_size : length;
            // запишем даннные в сектор
            if(write_sector(
                    start_sector,
                    buffer + written_bytes,
                    fragment_data_length,
                    length > vfs_header.sector_data_size ? false : true) != OK) {
                return VFS_FILE_READ_ERROR;
            }
            // уменьшим длину оставшихся данных
            length -= fragment_data_length;
            // увеличим число записанных байтов
            written_bytes += fragment_data_length;
            // проверяем ошибочный вариант записи
            if(length > 0 && start_sector == END_SECTOR) {
                return VFS_FILE_READ_ERROR;
            }
        }
        return OK;
    }

    /** \brief Очистить сектор
     * Данная функция заполнит список пустых секторов указанным сектором
     * и пометит ссылку данного сектора как конечную
     * \param sector сектор
     * \return вернет 0 в случае успеха, иначе код ошибки
     */
    int fvs_clean_sector(unsigned long &sector) {
        unsigned long file_offset = vfs_header.get_offset(sector);
        if(!check_offset(file_offset)) return VFS_FILE_READ_ERROR;
        if(vfs_header.empty_sectors.size() == 0) {
            vfs_header.empty_sectors.push_back(sector);
        } else {
            auto empty_sectors_it = std::upper_bound(
                vfs_header.empty_sectors.begin(),
                vfs_header.empty_sectors.end(),
                sector);
            if(*empty_sectors_it != sector) {
                vfs_header.empty_sectors.insert(empty_sectors_it, sector);
            }
        }
        if(fvs_get_next_sector(file_offset, sector) != OK) return VFS_FILE_READ_ERROR;
        if(fvs_set_next_sector(file_offset, END_SECTOR) != OK) return VFS_FILE_READ_ERROR;
        return OK;
    }

    /** \brief Очистить сектора
     * \param start_sector начальный сектор
     * \return вернет 0 в случае успеха, иначе код ошибки
     */
    int fvs_clean_sectors(unsigned long start_sector) {
        while(start_sector != END_SECTOR) {
            if(fvs_clean_sector(start_sector) != OK) return VFS_FILE_READ_ERROR;
        }
        return OK;
    }

    int fvs_read_header() {
        if(!fvs_file.seekg(0, std::ios::beg)) return false;
        fvs_file.clear();
        // читаем размер сектора
        unsigned long sector_size = 0;
        if(!fvs_file.read(reinterpret_cast<char *>(&sector_size), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
        // инициализируем заголовок и читаем все сектора заголовка
        vfs_header.init(fvs_file, sector_size);
        unsigned long offset = sizeof(unsigned long);
        // читаем файлы
        int state = 0;
        const int READ_FILES = 0;
        const int READ_FILE_ID = 1;
        const int READ_FILE_SIZE = 2;
        const int READ_FILE_START_SECTOR = 3;
        const int READ_EMPTY_SECTORS = 4;
        const int READ_EMPTY_SECTOR = 5;

        unsigned long files = 0;
        unsigned long empty_sectors = 0;
        unsigned long empty_sector = 0;
        header_file file_header;

        unsigned long sector = HEADER_START_SECTOR;
        while(sector != END_SECTOR) {
            if(offset == vfs_header.sector_data_size) {
                // относительное смещение находится на уровне ссылки на сл. сектор
                if(!fvs_file.read(reinterpret_cast<char *>(&sector), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                offset = 0;
                unsigned long file_offset = vfs_header.get_offset(sector);
                if(!check_offset(file_offset)) return VFS_FILE_READ_ERROR;
                if(!fvs_seekg_beg(file_offset)) return VFS_FILE_READ_ERROR;
            }
            switch(state) {
                case READ_FILES:
                    if(!fvs_file.read(reinterpret_cast<char *>(&files), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    state = READ_FILE_ID;
                break;
                case READ_FILE_ID:
                    if(!fvs_file.read(reinterpret_cast<char *>(&file_header.unique_id), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    state = READ_FILE_SIZE;
                    offset += sizeof(unsigned long); // так как id занимает 64 байт,  а не 32
                break;
                case READ_FILE_SIZE:
                    if(!fvs_file.read(reinterpret_cast<char *>(&file_header.file_size), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    state = READ_FILE_START_SECTOR;
                break;
                case READ_FILE_START_SECTOR:
                    if(!fvs_file.read(reinterpret_cast<char *>(&file_header.start_sector), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    vfs_header.files.push_back(file_header);
                    files--;
                    if(files == 0) state = READ_EMPTY_SECTORS;
                    else state = READ_FILE_ID;
                break;
                case READ_EMPTY_SECTORS:
                    if(!fvs_file.read(reinterpret_cast<char *>(&empty_sectors), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    state = READ_EMPTY_SECTOR;
                break;
                case READ_EMPTY_SECTOR:
                    if(!fvs_file.read(reinterpret_cast<char *>(&empty_sector), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    vfs_header.empty_sectors.push_back(empty_sector);
                    empty_sectors--;
                    if(empty_sectors == 0) return OK;
                    state = READ_EMPTY_SECTOR;
                break;
            }
            offset += sizeof(unsigned long);
        }
        return VFS_FILE_READ_ERROR;
    }

    int fvs_write_header() {
        if(!fvs_file.seekg(0, std::ios::beg)) return false;
        fvs_file.clear();

        if(!fvs_file.write(reinterpret_cast<char *>(&vfs_header.sector_size), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
        unsigned long offset = sizeof(unsigned long);
        // читаем файлы
        int state = 0;
        const int WRITE_FILES = 0;
        const int WRITE_FILE_ID = 1;
        const int WRITE_FILE_SIZE = 2;
        const int WRITE_FILE_START_SECTOR = 3;
        const int WRITE_EMPTY_SECTORS = 4;
        const int WRITE_EMPTY_SECTOR = 5;

        unsigned long files = vfs_header.files.size();
        unsigned long empty_sectors = vfs_header.empty_sectors.size();
        header_file file_header;

        unsigned long sector = HEADER_START_SECTOR;
        while(sector != END_SECTOR) {
            if(offset == vfs_header.sector_data_size) {
                // относительное смещение находится на уровне ссылки на сл. сектор
                if(!fvs_file.read(reinterpret_cast<char *>(&sector), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                offset = 0;
                unsigned long file_offset = vfs_header.get_offset(sector);
                if(!check_offset(file_offset)) return VFS_FILE_READ_ERROR;
                if(!fvs_seekg_beg(file_offset)) return VFS_FILE_READ_ERROR;
            }
            switch(state) {
                case WRITE_FILES:
                    if(!fvs_file.write(reinterpret_cast<char *>(&files), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    state = READ_FILE_ID;
                break;
                case WRITE_FILE_ID:
                    if(!fvs_file.write(reinterpret_cast<char *>(&file_header.unique_id), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    state = READ_FILE_SIZE;
                break;
                case WRITE_FILE_SIZE:
                    if(!fvs_file.write(reinterpret_cast<char *>(&file_header.file_size), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    state = READ_FILE_START_SECTOR;
                break;
                case WRITE_FILE_START_SECTOR:
                    if(!fvs_file.write(reinterpret_cast<char *>(&file_header.start_sector), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    vfs_header.files.push_back(file_header);
                    files--;
                    if(files == 0) state = READ_EMPTY_SECTORS;
                    else state = READ_FILE_ID;
                break;
                case WRITE_EMPTY_SECTORS:
                    if(!fvs_file.write(reinterpret_cast<char *>(&empty_sectors), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    state = READ_EMPTY_SECTOR;
                break;
                case WRITE_EMPTY_SECTOR:
                    if(!fvs_file.write(reinterpret_cast<char *>(&empty_sector), sizeof(unsigned long))) return VFS_FILE_READ_ERROR;
                    vfs_header.empty_sectors.push_back(empty_sector);
                    empty_sectors--;
                    if(empty_sectors == 0) return OK;
                    state = READ_EMPTY_SECTOR;
                break;
            }
            offset += sizeof(unsigned long);
        }
        return VFS_FILE_READ_ERROR;
    }

    int write_file(unsigned long long unique_id, char *buffer, unsigned long length) {
        // ищем файл

    }

    public:

    xvfs(std::string file_name, const unsigned long  sector_size = 32) {
        if(!fvs_create_file(file_name)) return;
        if(!fvs_open_file(file_name)) return;
        vfs_header.init(fvs_file, sector_size);
        /*
        if(!check_file(file_name)) {
            if(!create_file(file_name)) return;
            if(!open_file(file_name)) return;
            xvfs_header.init(fvs_file, sector_size);
        } else {
            open_file(file_name);
        }
        */
    }
};

#endif // XVFS_HPP_INCLUDED
