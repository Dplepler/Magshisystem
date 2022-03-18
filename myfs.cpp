#include "myfs.h"
#include <string.h>
#include <iostream>
#include <math.h>
#include <sstream>

const char *MyFs::MYFS_MAGIC = "MYFS";

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_):blkdevsim(blkdevsim_) {
	struct myfs_header header;
	blkdevsim->read(0, sizeof(header), (char *)&header);

	if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
	    (header.version != CURR_VERSION)) {
		std::cout << "Did not find myfs instance on blkdev" << std::endl;
		std::cout << "Creating..." << std::endl;
		format();
		std::cout << "Finished!" << std::endl;
	}
}

void MyFs::format() {

	// put the header in place
	struct myfs_header header;
	strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
	header.version = CURR_VERSION;
	blkdevsim->write(0, sizeof(header), (const char*)&header);

	// TODO: put your format code here
}

void MyFs::create_file(std::string path_str, bool directory) {
	
	char buffer[2];

	if (file_already_exists(path_str.c_str()))

	/* Iterate through all the data available, and find an empty block to start the file from */
	for (unsigned int i = 0; i < BLOCK_DEVICE_SIZE - DATA_OFFSET; i += BLOCK_SIZE) {

		if (this->blkdevsim->read(i, sizeof(bool), buffer) {

		}

	}


}

std::string MyFs::get_content(std::string path_str) {
	throw std::runtime_error("not implemented");
	return "";
}

void MyFs::set_content(std::string path_str, std::string content) {
	throw std::runtime_error("not implemented");
}

MyFs::dir_list MyFs::list_dir(std::string path_str) {
	dir_list ans;
	throw std::runtime_error("not implemented");
	return ans;
}

/*
file_already_exists checks if a filename already exists in the current block device
Input: Filename to search for
Output: True if filename was found, otherwise false
*/
bool MyFs::file_already_exists(char* filename) {

	inode_t inode;

	for (unsigned int i = 0; i < DATA_OFFSET; i += sizeof(inode_t)) {

		this->blkdevsim->read(i, sizeof(inode), &(char*)inode);
		if (!strcmp(inode.filename, filename)) {
			return true;
		}
	}

	return false;
}

