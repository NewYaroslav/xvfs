#include <iostream>
#include "xvfs.hpp"

int main()
{
    std::cout << "Hello world!" << std::endl;
    char test_data[256];
    for(int i = 0; i < 256; ++i) {
        test_data[i] = i;
    }
    xvfs iVfs("test.dat", 32);
    std::cout << "xvfs vfs_file_size: " << iVfs.vfs_header.vfs_file_size << std::endl;
    std::cout << "xvfs sector_size: " << iVfs.vfs_header.sector_size << std::endl;
    std::cout << "xvfs sector_data_size: " << iVfs.vfs_header.sector_data_size << std::endl;
    int err = iVfs.write_sectors(0, test_data, 64);
    std::cout << "write_sectors " << err << std::endl;
    int err2 = iVfs.write_sectors(3, test_data, 64);
    std::cout << "write_sectors " << err2 << std::endl;
    int err3 = iVfs.write_sectors(0, test_data, 64+32);
    std::cout << "write_sectors " << err3 << std::endl;

    int err4 = iVfs.fvs_clean_sectors(0);
    std::cout << "clean_sectors " << err4 << std::endl;
    return 0;
}
