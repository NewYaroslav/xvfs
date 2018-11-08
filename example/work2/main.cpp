#include <iostream>
#include <cstdlib>
#include <xvfs.hpp>

using namespace std;

int main() {
    std::cout << "start test" << std::endl;
    xvfs VFS("test.hex", 48, xvfs::USE_LZ4);
    if(VFS.is_open()) cout << "is_open" << endl;
    // тестовые данные
    int num_test = 0;
    while(num_test < 2) {
        num_test++;
        for(int i = 0; i < 10; ++i) {
            long max_size = 1000 + (rand() % 100) * 100;
            char* test_data = new char[max_size];
            for(int i = 0; i < max_size; ++i) {
                test_data[i] = rand() % 200;
            }
            if(VFS.write_file("xxx_" + std::to_string(i), test_data, max_size)) cout << "write xxx_" + std::to_string(i) + " ok" << endl;
            else { cout << "write xxx_" + std::to_string(i) + " error" << endl; return 0; };
            delete[] test_data;
        }

        for(int i = 0; i < 10; ++i) {
            long max_size = 1000 + (rand() % 100) * 100;
            char* test_data = new char[max_size];
            for(int i = 0; i < max_size; ++i) {
                test_data[i] = rand() % 200;
            }
            if(VFS.write_file("xxx_" + std::to_string(i), test_data, max_size)) cout << "write xxx_" + std::to_string(i) + " ok" << endl;
            else { cout << "write xxx_" + std::to_string(i) + " error" << endl; return 0; };
            delete[] test_data;
        }

        for(int i = 0; i < 100; ++i) {
            long max_size = 1000 + (rand() % 100) * 100;
            char* test_data = new char[max_size];
            for(int i = 0; i < max_size; ++i) {
                test_data[i] = rand() % 200;
            }
            if(VFS.write_file("xxx_" + std::to_string(i), test_data, max_size)) cout << "write xxx_" + std::to_string(i) + " ok" << endl;
            else { cout << "write xxx_" + std::to_string(i) + " error" << endl; return 0; };
            delete[] test_data;
        }

        for(int i = 0; i < 100; ++i) {
            long max_size = (rand() % 100) * 1000;
            char* test_data = new char[max_size];
            for(int i = 0; i < max_size; ++i) {
                test_data[i] = rand() % 20;
            }
            if(VFS.write_file("xxx_" + std::to_string(i), test_data, max_size)) cout << "write xxx_" + std::to_string(i) + " ok" << endl;
            else { cout << "write xxx_" + std::to_string(i) + " error" << endl; return 0; };
            delete[] test_data;
        }

        for(int i = 0; i < 100; ++i) {
            long max_size = (rand() % 100) * 1000;
            char* test_data = new char[max_size];
            for(int i = 0; i < max_size; ++i) {
                test_data[i] = rand() % 50;
            }
            if(VFS.write_file("xxx_" + std::to_string(i), test_data, max_size)) cout << "write xxx_" + std::to_string(i) + " ok" << endl;
            else { cout << "write xxx_" + std::to_string(i) + " error" << endl; return 0; };
            delete[] test_data;
        }

        for(int i = 0; i < 100; ++i) {
            long max_size = (rand() % 100) * 1000;
            char* test_data = new char[max_size];
            for(int i = 0; i < max_size; ++i) {
                test_data[i] = rand() % 5;
            }
            if(VFS.write_file("xxx_" + std::to_string(i), test_data, max_size)) cout << "write xxx_" + std::to_string(i) + " ok" << endl;
            else { cout << "write xxx_" + std::to_string(i) + " error" << endl; return 0; };
            delete[] test_data;
        }


        for(int i = 0; i < 100; ++i) {
            long max_size = (rand() % 100) * 1000;
            std::cout << "max_size " << max_size << std::endl;
            if(!VFS.open("xxx_" + std::to_string(i), VFS.WRITE_FILE)) { cout << "open xxx_" + std::to_string(i) + " error" << endl; return 0; }
            else  cout << "open xxx_" + std::to_string(i) << endl;
            if(!VFS.reserve_memory(max_size)) { cout << "reserve_memory xxx_" + std::to_string(i) + " error" << endl; return 0; }

            for(int j = 0; j < max_size; ++j) {
                char temp = rand() % 5;
                long len = VFS.write(&temp, 1);
                if(len != 1) { cout << "write xxx_" + std::to_string(i) + " error " << j << endl; return 0; };
            }

            if(!VFS.close()) { cout << "close xxx_" + std::to_string(i) + " error" << endl; return 0; };

            // читаем
            char* read_data = NULL;
            long file_size = VFS.read_file("xxx_" + std::to_string(i), read_data);
            if(file_size < 0) cout << "error read xxx_" + std::to_string(i) + " size: " << file_size << endl;
            else {
                cout << "read xxx_" + std::to_string(i) + " ok" << endl;
            }
            if(read_data != NULL) delete[] read_data;
        }



        for(int i = 0; i < 100; ++i) {
            long max_size = 1000 + (rand() % 100) * 100;
            char* test_data = new char[max_size];
            for(int i = 0; i < max_size; ++i) {
                test_data[i] = rand() % 4;
            }
            if(VFS.write_file("xxx_" + std::to_string(i), test_data, max_size)) cout << "write xxx_" + std::to_string(i) + " ok" << endl;
            else { cout << "write xxx_" + std::to_string(i) + " error" << endl; return 0; };

            // читаем
            char* read_data = NULL;
            long file_size = VFS.read_file("xxx_" + std::to_string(i), read_data);
            if(file_size < 0) cout << "error read xxx_" + std::to_string(i) + " size: " << file_size << endl;
            else {
                cout << "read xxx_" + std::to_string(i) + " ok" << endl;
                for(int j = 0; j < file_size; ++j) {
                    if(read_data[j] != test_data[j]) {cout << "error !=" << endl; return 0; }
                }
            }
            if(read_data != NULL) delete[] read_data;
            delete[] test_data;
        }

        for(int i = 0; i < 10; ++i) {
            if(VFS.delete_file("xxx_" + std::to_string(i))) cout << "delete xxx_" + std::to_string(i) + " ok" << endl;
            else cout << "delete xxx_" + std::to_string(i) + " error" << endl;
        }

    }

    unsigned long sector_size = 0;
    long compression_type = 0;
    std::vector<xvfs::_xvfs_file_header> files;
    std::vector<unsigned long> empty_sectors;
    VFS.get_info(sector_size, compression_type, files, empty_sectors);

    cout << "sector size " << sector_size << endl;
    cout << "compression type " << compression_type << endl;
    cout << "files " << files.size() << endl;
    for(size_t i = 0; i < files.size(); ++i) {
        cout << "*" << endl;
        cout << "files hash: " << files[i].hash << endl;
        cout << "files compress size: " << files[i].size << endl;
        cout << "files uncompress size: " << files[i].real_size << endl;
        cout << "files start sector: " << files[i].start_sector << endl;
    }
    cout << "empty_sectors " << empty_sectors.size() << endl;
#if(0)
    for(size_t i = 0; i < empty_sectors.size(); ++i) {
        cout << "sectors: " << empty_sectors[i] << endl;
    }
#endif
    return 0;
}
