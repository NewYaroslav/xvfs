# xvfs
Виртуальная файловая система

## Как это работает?
Создается файл, содержащий внутри себя виртуальную файловую систему, т.е. данный файл может содержать в себе множество "виртуальных" файлов. Каждый "виртуальный" файл по сути представляет из себя обычный массив данных. "Виртуальные" файлы можно читать, записывать, удалять из виртуальной файловой системы. Виртуальная файловая система (далее VFS) состоит из секторов. Размер секторов можно задать вручную перед созданием файла VFS. В конце каждого сектора отводится 4 байта под ссылку на следующий сектор, из которого можно читать или в который нужно записывать данные. Если при записи файла сектор оказался последним, в ссылке указывается число 0xFFFFFFFF. Каждый файл имеет свой уникальный id, начальный сектор считывания и данные о реальном и сжатом размере файла. Эти данные хранятся в заголовке файла VFS. Для имитации работы с именами файлов и папок класс VFS содержит функции, которые преобразуют строкове имя (путь к файлу) в хэш, который используется как уникальный id для виртуального файла.

## Особенности виртуальной файловой системы
+ Для быстрого поиска файла в заголовке используется бинарный поиск по его id
+ VFS разбивает все пространство на сектора
+ VFS запоминает пустые сектора и использует их в первую очередь при записи новых файлов или расширении размера уже существующих файлов
+ VFS не использует "папок" и строковых имен файлов, все файлы представляют из себя отсортированный массив уникальных id с данными первого сектора файла и размера файла
+ VFS может читать или записывать файл только целиком, доступ к отдельным байтам виртуальных файлов на данный момент не реализован
+ Можно сжимать файлы (для компрессии и декопресии используется zlib)

## Как установить?
Для начала использования VFS достаточно просто добавить в свой проект файлы src/xvfs.hpp и src/xvfs.cpp. Если необходимо использовать сжатие виртуальных файлов, добавьте в проект библиотеку zlib и добавьте макрос XFVS_USE_ZLIB.

## Почему нет полноценной работы с файлами как в настоящей файловой системе?
Потому что для меня такой задачи не стояло и мне достаточно использовать такое решение

## Пример использования
```
#include <xvfs.hpp>

...
/* откроем или создадим файл vfs с размером сектора 512 байт
 * Для использования сжатия файлов используйте коснтруктор с указанием типа компрессии
 * xvfs VFS("test.hex", 512, Z_BEST_COMPRESSION);
 */
xvfs VFS("test.hex", 512);
// узнаем, получилось ли прочитать или создать файл VFS
if(VFS.is_open()) cout << "is_open" << endl;
// запишем что нибудь
int test_size_1 = 4567;
char test_data_1[4567];
for(int i = 0; i < test_size_1; ++i) {
    test_data_1[i] = rand() % 255; // заполним массив случайными данными
}
/* создадим и запишем (или перезапишем) в файл "test_1
  Как вариант, можно использовать функцию VFS.write_file, которая
  вместо имени файла принимает значение его id, например:
  const int test_id = 1245;
  
  VFS.write_file(test_id, test_data_1, test_size_1);
*/
if(VFS.write_file("test_1", test_data_1, test_size_1)) cout << "write test_1 ok" << endl;
else cout << "write test_1 error" << endl;

// теперь прочитаем содержимое
char* read_data_1 = NULL;
long file_size_1 = VFS.read_file("test_1", read_data_1);
if(file_size_1 != - 1) cout << "read test_1, size: " << file_size_1 << endl;
else cout << "read test_1 error" << endl;
...

...
delete[] read_data_1;
read_data_1 = NULL;
...
// теперь удалим файл
if(VFS.delete_file("test_1")) cout << "delete test_3 ok" << endl;
else cout << "delete test_3 error" << endl;
// получим информацию о VFS
unsigned long sector_size = 0;
std::vector<xvfs::_xvfs_file_header> files;
std::vector<unsigned long> empty_sectors;
VFS.get_info(sector_size, files, empty_sectors);
// выведем информацию на экран
cout << "sector size " << sector_size << endl;
cout << "files " << files.size() << endl;
for(size_t i = 0; i < files.size(); ++i) {
  cout << "*" << endl;
  cout << "files hash: " << files[i].hash << endl; // уникальный id файла
  cout << "files compress size: " << files[i].size << endl; // размер виртуального файла в VFS (после компрессии)
  cout << "files uncompress size: " << files[i].real_size << endl; // реальный размер файла (после декомпресии)
  cout << "files start sector: " << files[i].start_sector << endl; // первый сектор файла
}
cout << "empty_sectors " << empty_sectors.size() << endl; // количество пустых секторов
for(size_t i = 0; i < empty_sectors.size(); ++i) {
  cout << "sectors: " << empty_sectors[i] << endl; // пустой сектор
}
```
