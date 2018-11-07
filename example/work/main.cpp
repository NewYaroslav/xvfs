#include <iostream>
#include <cstdlib>
#include <xvfs.hpp>

using namespace std;

int main() {
    std::cout << "start test" << std::endl;
    xvfs VFS("test2.hex", 48, xvfs::USE_ZLIB_LEVEL_9);
    if(VFS.is_open()) cout << "is_open" << endl;
    // тестовые данные
    int test_size_1 = 347;
    char test_data_1[347];

    int test_size_2 = 333444;
    char test_data_2[333444];

    int test_size_3 = 1200;
    char test_data_3[1200];

    int test_size_4 = 0;

    for(int i = 0; i < test_size_1; ++i) {
        test_data_1[i] = rand() % 2;
    }
    for(int i = 0; i < test_size_2; ++i) {
        test_data_2[i] = rand() % 3;
    }
    for(int i = 0; i < test_size_3; ++i) {
        test_data_3[i] = rand() % 4;
    }

    //
    cout << "open file test_a" << endl;
    if(VFS.open("test_a", VFS.WRITE_FILE)) cout << "open test_a ok" << endl;
    else cout << "open test_a error" << endl;

    for(int i = 0; i < test_size_3; ++i) {
        char var = i % 10;
        VFS.write(&var, 1);
    }

    if(VFS.close()) cout << "close test_a ok" << endl;
    else cout << "close test_a error" << endl;
    //

    //
    cout << "open file test_a" << endl;
    if(VFS.open("test_a", VFS.WRITE_FILE)) cout << "open test_a ok" << endl;
    else cout << "open test_a error" << endl;

    for(int i = 0; i < 50000; ++i) {
        char var = i % 10;
        VFS.write(&var, 1);
    }

    if(VFS.close()) cout << "close test_a ok" << endl;
    else cout << "close test_a error" << endl;
    //

    //
    cout << "open file test_b" << endl;
    if(VFS.open("test_b", VFS.WRITE_FILE)) cout << "open test_b ok" << endl;
    else cout << "open test_b error" << endl;

    for(int i = 0; i < 20000; ++i) {
        char var = i % 22;
        VFS.write(&var, 1);
    }

    if(VFS.close()) cout << "close test_b ok" << endl;
    else cout << "close test_b error" << endl;
    //

    for(int i = 0; i < 10; ++i) {
        if(VFS.write_file("xxx_" + std::to_string(i), test_data_1, test_size_1)) cout << "write xxx_" + std::to_string(i) + " ok" << endl;
        else cout << "write xxx_" + std::to_string(i) + " error" << endl;
    }

    cout << "write to file test_1" << endl;
    if(VFS.write_file("test_1", test_data_1, test_size_1)) cout << "write test_1 ok" << endl;
    else cout << "write test_1 error" << endl;

    cout << "write to file test_2" << endl;
    if(VFS.write_file("test_2", test_data_2, test_size_2)) cout << "write test_2 ok" << endl;
    else cout << "write test_2 error" << endl;

    cout << "write to file test_3" << endl;
    if(VFS.write_file("test_3", test_data_3, test_size_3)) cout << "write test_3 ok" << endl;
    else cout << "write test_3 error" << endl;

    cout << "write to file test_4" << endl;
    if(VFS.write_file("test_4", test_data_3, test_size_4)) cout << "write test_4 ok" << endl;
    else cout << "write test_4 error" << endl;

    cout << "write to file test_1" << endl;
    if(VFS.write_file("test_1", test_data_3, test_size_3)) cout << "write test_1 ok" << endl;
    else cout << "write test_1 error" << endl;

    cout << "read file test_0" << endl;
    char* read_data = NULL;
    long file_size = VFS.read_file("test_0", read_data);
    if(file_size != - 1) cout << "read test_0 ok" << endl;
    else {
        cout << "read test_0 error" << endl;
        delete[] read_data;
        read_data = NULL;
    }

    cout << "read file test_a" << endl;
    char* read_data_a = NULL;
    long file_size_a = VFS.read_file("test_a", read_data_a);
    if(file_size_a != - 1) cout << "read test_a, size: " << file_size_a << endl;
    else cout << "read test_a error" << endl;

    cout << "read file test_1" << endl;
    char* read_data_1 = NULL;
    long file_size_1 = VFS.read_file("test_1", read_data_1);
    if(file_size_1 != - 1) cout << "read test_1, size: " << file_size_1 << endl;
    else cout << "read test_1 error" << endl;

    cout << "read file test_2" << endl;
    char* read_data_2 = NULL;
    long file_size_2 = VFS.read_file("test_2", read_data_2);
    if(file_size_2 != - 1) cout << "read test_2, size: " << file_size_2 << endl;
    else cout << "read test_2 error" << endl;

    cout << "read file test_3" << endl;
    char* read_data_3 = NULL;
    long file_size_3 = VFS.read_file("test_3", read_data_3);
    if(file_size_3 != - 1) cout << "read test_3, size: " << file_size_3 << endl;
    else cout << "read test_3 error" << endl;

    cout << "read file test_4" << endl;
    char* read_data_4 = NULL;
    long file_size_4 = VFS.read_file("test_4", read_data_4);
    if(file_size_4 != - 1) cout << "read test_4, size: " << file_size_4 << endl;
    else cout << "read test_3 error" << endl;


    cout << "test read_data..." << endl;
    for(int i = 0; i < file_size_1; ++i) {
        if(read_data_1[i] != test_data_3[i]) {
            cout << "error read_data_1[" << i << "] != test_data_3" << endl;
            break;
        }
    }

    for(int i = 0; i < file_size_2; ++i) {
        if(read_data_2[i] != test_data_2[i]) {
            cout << "error read_data_2[" << i << "] != test_data_2" << endl;
            break;
        }
    }

    for(int i = 0; i < file_size_3; ++i) {
        if(read_data_3[i] != test_data_3[i]) {
            cout << "error read_data_3[" << i << "] != test_data_3" << endl;
            break;
        }
    }

    //
    cout << "open file test_b" << endl;
    if(VFS.open("test_b", VFS.READ_FILE)) cout << "open test_b ok" << endl;
    else cout << "open test_b error" << endl;

    long len_test_b = VFS.get_size();
    cout << "read file test_b" << endl;
    for(int i = 0; i < len_test_b; ++i) {
        char temp = i % 22;
        char var;
        long len = VFS.read(&var, 1);
        if(len != 1) std::cout << "error " << len << std::endl;
        if(var != temp) std::cout << "error var != temp" << std::endl;
    }

    if(VFS.close()) cout << "close test_b ok" << endl;
    else cout << "close test_b error" << endl;
    //

    if(read_data_a != NULL) delete[] read_data_a;
    if(read_data_1 != NULL) delete[] read_data_1;
    if(read_data_2 != NULL) delete[] read_data_2;
    if(read_data_3 != NULL) delete[] read_data_3;

    if(VFS.delete_file("test_3")) cout << "delete test_3 ok" << endl;
    else cout << "delete test_3 error" << endl;

    if(VFS.delete_file("test_3")) cout << "delete test_3 ok" << endl;
    else cout << "delete test_3 error" << endl;

    if(VFS.delete_file("test_4")) cout << "delete test_4 ok" << endl;
    else cout << "delete test_4 error" << endl;

    file_size_3 = VFS.read_file("test_3", read_data_3);
    if(file_size != - 1) cout << "read test_3, size: " << file_size_3 << endl;
    else cout << "read test_3 error" << endl;


    for(int i = 0; i < 10; ++i) {
        if(VFS.delete_file("xxx_" + std::to_string(i))) cout << "delete xxx_" + std::to_string(i) + " ok" << endl;
        else cout << "delete xxx_" + std::to_string(i) + " error" << endl;
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
