#ifndef XVFS_HPP_INCLUDED
#define XVFS_HPP_INCLUDED

#include <string>
#include <vector>

class xvfs {
public:

    /** \brief Структура заголовка виртуального файла
     */
    struct _xvfs_file_header {
        long long hash;                                 /**< Хэш файла */
        unsigned long size;                             /**< Размер файла */
        unsigned long start_sector;                     /**< Начальный сектор файла */

        _xvfs_file_header() {};

        _xvfs_file_header(long long _hash, unsigned long _size, unsigned long _start_sector) {
            hash = _hash;
            size = _size;
            start_sector = _start_sector;
        }

        bool operator<(const _xvfs_file_header& value)const{return hash < value.hash;}

    };

private:

    long binary_search_first(std::vector<_xvfs_file_header> arr, long long key, long left, long right);

    /** \brief Структура заголовка
     */
    struct _xvfs_header {
        unsigned long sector_size;                      /**< Размер сектора */
        std::vector<_xvfs_file_header> files;           /**< Файлы */
        std::vector<unsigned long> empty_sectors;       /**< Пустые сектора */
        unsigned long get_size() {
            unsigned long size = sizeof(sector_size) +
                3 * sizeof(unsigned long) + // размер раголовка, количества файлов, пустых секторов
                files.size() * sizeof(_xvfs_file_header) + // объем всех файлов
                empty_sectors.size() * sizeof(unsigned long); // пустые сектора
            return size;
        }
    } xvfs_header;

    bool is_open_file = false;                          /**< Файл виртуальной файловой системы открыт или нет */
    std::string file_name;                              /**< Имя файла виртуальной файловой системы */
    //unsigned long sector_size = 512;                    /**< Размер сектора */

    bool check_file(std::string file_name);
    bool create_file(std::string file_name);
    long read_data(unsigned long start_sector, char* file_data, unsigned long file_size);
    long write_data(unsigned long start_sector, char* file_data, unsigned long file_size);

    bool read_header();
    bool save_header();

    bool clear_data(unsigned long start_sector, bool is_first_sector);
    unsigned long get_last_new_sector();

    void init_xvfs_header();
    void init_xvfs_header(int sector_size);

    const long long poly = 0xC96C5795D7870F42;
    void generate_table();
public:

    /** \brief Инициализировать виртуальную файловую систему
     * Данный конструктор открывает или создает файл виртуальноц файловой системы с размером сектора 512 байт
     * \param file_name имя файла виртуальной файловой системы
     */
    xvfs(std::string file_name);

    /** \brief Инициализировать виртуальную файловую систему
     * Данный конструктор открывает или создает файл виртуальноц файловой системы с размером сектора sector_size байт
     * \param file_name имя файла виртуальной файловой системы
     * \param sector_size желаемый размер сектора при создании файла
     */
    xvfs(std::string file_name, int sector_size);

    /** \brief Состояние файла виртуальной файловой системы
     * \return вернет true если файл виртуальной файловой системы был открыт или создан
     */
    inline bool is_open() {return is_open_file;};

    /** \brief Записать в файл
     * \param vfs_file_name имя файла
     * \param data данные
     * \param len длина файла
     * \return вернет true в случае успеха
     */
    bool write_file(std::string vfs_file_name, char* data, unsigned long len);

    /** \brief Записать в файл
     * \param hash_vfs_file хэш файла
     * \param data данные
     * \param len длина файла
     * \return вернет true в случае успеха
     */
    bool write_file(long long hash_vfs_file, char* data, unsigned long len);

    /** \brief Читать файл
     * Функция сама выделяет память под данные
     * \param vfs_file_name имя файла
     * \param data данные
     * \param len длина файла
     * \return вернет длину файла в случае успеха или -1 в случае ошибки
     */
    long read_file(std::string vfs_file_name, char*& data);

    /** \brief Читать файл
     * Функция сама выделяет память под данные
     * \param hash_vfs_file хэш файла
     * \param data данные
     * \param len длина файла
     * \return вернет длину файла в случае успеха или -1 в случае ошибки
     */
    long read_file(long long hash_vfs_file, char*& data);

    /** \brief Получить длину файла
     * \param vfs_file_name имя файла
     * \return длина файла в случае успеха или -1 в случае ошибки
     */
    long get_len_file(std::string vfs_file_name);

    /** \brief Получить длину файла
     * \param hash_vfs_file хэш файла
     * \return длина файла в случае успеха или -1 в случае ошибки
     */
    long get_len_file(long long hash_vfs_file);

    /** \brief Удалить файл
     * \param vfs_file_name имя файла
     * \return вернет true в случае успеха
     */
    bool delete_file(std::string vfs_file_name);

    /** \brief Удалить файл
     * \param hash_vfs_file хэш файла
     * \return вернет true в случае успеха
     */
    bool delete_file(long long hash_vfs_file);

    /** \brief Получить информацию о файлах
     * \param sector_size размер сектора
     * \param files файлы
     * \param empty_sectors пустые секторы
     */
    inline void get_info(unsigned long& sector_size, std::vector<_xvfs_file_header>& files, std::vector<unsigned long>& empty_sectors) {
        sector_size = xvfs_header.sector_size;
        files = xvfs_header.files;
        empty_sectors = xvfs_header.empty_sectors;
    }

    long long calculate_crc64(long long crc, const unsigned char* stream, int n);

    long long calculate_crc64(std::string vfs_file_name);
};

#endif // XVFS_HPP_INCLUDED
